/*****************************************************************************\
 *  src/interfaces/cred.h - Slurm job and sbcast credential functions
 *****************************************************************************
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2010 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Mark Grondona <grondona1@llnl.gov>.
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#ifndef _HAVE_SLURM_CRED_H
#define _HAVE_SLURM_CRED_H

#include <sys/types.h>
#include <unistd.h>

#include "src/common/bitstring.h"
#include "src/common/macros.h"
#include "src/common/pack.h"

/*
 * Default credential information expiration window.
 * Long enough for loading user environment, running prolog, paging slurmd
 * into memory, plus sending a launch request to all compute nodes of a job
 * (i.e. MessageTimeout * message_depth, where
 * (TreeWidth ^^ message_depth) >= count_of_compute_nodes).
 *
 * The default value may be altered with the configuration option of this sort:
 * "AuthInfo=cred_expire=600"
 */
#define DEFAULT_EXPIRATION_WINDOW 120

/*
 * The incomplete slurm_cred_t type is also defined in slurm.h for
 * users of the api, so check to ensure that this header has not been
 * included after slurm.h:
 */
#ifndef __slurm_cred_t_defined
#  define __slurm_cred_t_defined
   typedef struct slurm_job_credential slurm_cred_t;
#endif

/*
 * The incomplete slurm_cred_t type is also defined in slurm_protocol_defs.h
 * so check to ensure that this header has not been included after
 * slurm_protocol_defs.h:
 */
#ifndef __sbcast_cred_t_defined
#  define  __sbcast_cred_t_defined
   typedef struct sbcast_cred *sbcast_cred_t;		/* opaque data type */
#endif

/*
 * The slurm_cred_ctx_t incomplete type
 */
typedef struct slurm_cred_context   * slurm_cred_ctx_t;

/* Used by slurm_cred_get() */
typedef enum {
	CRED_DATA_JOB_GRES_LIST = 1,
	CRED_DATA_JOB_ALIAS_LIST,
	CRED_DATA_STEP_GRES_LIST,
} cred_data_enum_t;

/*
 * Initialize current process for slurm credential creation.
 *
 * `privkey' contains the absolute path to the slurmctld private
 * key, which needs to be readable by the current process.
 *
 * Returns 0 for success, -1 on failure and sets errno to reason.
 *
 *
 */
slurm_cred_ctx_t slurm_cred_creator_ctx_create(const char *privkey);

/*
 * Initialize current process for slurm credential verification.
 * `pubkey' contains the absolute path to the slurmctld public key.
 *
 * Returns 0 for success, -1 on failure.
 */
slurm_cred_ctx_t slurm_cred_verifier_ctx_create(const char *pubkey);

/*
 * Set and get credential context options
 *
 */
typedef enum {
	SLURM_CRED_OPT_EXPIRY_WINDOW /* expiration time of creds (int );  */
} slurm_cred_opt_t;

int slurm_cred_ctx_set(slurm_cred_ctx_t ctx, slurm_cred_opt_t opt, ...);
int slurm_cred_ctx_get(slurm_cred_ctx_t ctx, slurm_cred_opt_t opt, ...);

/*
 * Update the context's current key.
 */
int slurm_cred_ctx_key_update(slurm_cred_ctx_t ctx, const char *keypath);


/*
 * Destroy a credential context, freeing associated memory.
 */
void slurm_cred_ctx_destroy(slurm_cred_ctx_t ctx);

/*
 * Pack and unpack slurm credential context.
 *
 * On pack() ctx is packed in machine-independent format into the
 * buffer, on unpack() the contents of the buffer are used to
 * initialize the state of the context ctx.
 */
int slurm_cred_ctx_pack(slurm_cred_ctx_t ctx, buf_t *buffer);
int slurm_cred_ctx_unpack(slurm_cred_ctx_t ctx, buf_t *buffer);


/*
 * Container for Slurm credential create/fetch/verify arguments
 *
 * The core_bitmap, cores_per_socket, sockets_per_node, and
 * sock_core_rep_count is based upon the nodes allocated to the
 * JOB, but the bits set in core_bitmap are those cores allocated
 * to this STEP
 */
typedef struct {
	slurm_step_id_t step_id;
	uid_t uid; /* user for which the cred is valid */
	gid_t gid; /* user's primary group id */
	/*
	 * These are only used in certain conditions and should not be supplied
	 * when creating a new credential.  They are defined here so the values
	 * can be fetched from the credential.
	 */
	char *pw_name; /* user_name as a string */
	char *pw_gecos; /* user information */
	char *pw_dir; /* home directory */
	char *pw_shell; /* user program */
	int ngids; /* number of extended group ids */
	gid_t *gids; /* extended group ids for user */
	char **gr_names; /* array of group names matching gids */

	/* job_core_bitmap and step_core_bitmap cover the same set of nodes,
	 * namely the set of nodes allocated to the job. The core and socket
	 * information below applies to job_core_bitmap AND step_core_bitmap */
	uint16_t core_array_size;	/* core/socket array size */
	uint16_t *cores_per_socket;	/* Used for job/step_core_bitmaps */
	uint16_t *sockets_per_node;	/* Used for job/step_core_bitmaps */
	uint32_t *sock_core_rep_count;	/* Used for job/step_core_bitmaps */

	uint32_t cpu_array_count;
	uint16_t *cpu_array;
	uint32_t *cpu_array_reps;

	/* JOB specific info */
	char *job_account;		/* account */
	char *job_alias_list;		/* node name to address aliases */
	char *job_comment;		/* comment */
	char     *job_constraints;	/* constraints in job allocation */
	bitstr_t *job_core_bitmap;	/* cores allocated to JOB */
	uint16_t  job_core_spec;	/* count of specialized cores */
	time_t job_end_time;            /* UNIX timestamp for job end time */
	char *job_extra;		/* Extra - arbitrary string */
	char     *job_hostlist;		/* list of nodes allocated to JOB */
	char *job_licenses;		/* Licenses allocated to job */
	uint64_t *job_mem_alloc;	/* Per node allocated mem in rep.cnt. */
	uint32_t *job_mem_alloc_rep_count;
	uint32_t job_mem_alloc_size;	/* Size of memory arrays above */
	uint32_t  job_nhosts;		/* count of nodes allocated to JOB */
	uint32_t job_ntasks;
	uint16_t job_oversubscribe;	/* shared/oversubscribe status */
	List job_gres_list;		/* Generic resources allocated to JOB */
	char *job_partition;		/* partition */
	char *job_reservation;		/* Reservation, if applicable */
	uint16_t job_restart_cnt;	/* restart count */
	time_t job_start_time;          /* UNIX timestamp for job start time */
	char *job_std_err;
	char *job_std_in;
	char *job_std_out;
	uint16_t  x11;			/* x11 flag set on job */

	char *selinux_context;

	/* STEP specific info */
	bitstr_t *step_core_bitmap;	/* cores allocated to STEP */
	char     *step_hostlist;	/* list of nodes allocated to STEP */
	uint64_t *step_mem_alloc;	/* Per node allocated mem in rep.cnt. */
	uint32_t *step_mem_alloc_rep_count;
	uint32_t step_mem_alloc_size;	/* Size of memory arrays above */

	List step_gres_list;		/* Generic resources allocated to STEP */

	void *switch_step;
} slurm_cred_arg_t;

/* Initialize the plugin. */
int slurm_cred_init(void);

/* Terminate the plugin and release all memory. */
int slurm_cred_fini(void);

/*
 * Create a slurm credential using the values in `arg.'
 * The credential is signed using the creators public key.
 *
 * `arg' must be non-NULL and have valid values. The arguments
 * will be copied as is into the slurm job credential.
 *
 * Returns NULL on failure.
 */
slurm_cred_t *slurm_cred_create(slurm_cred_ctx_t ctx, slurm_cred_arg_t *arg,
				bool sign_it, uint16_t protocol_version);

/*
 * Create a "fake" credential with bogus data in the signature.
 * This function can be used for testing, or when srun would like
 * to talk to slurmd directly, bypassing the controller
 * (which normally signs creds)
 */
slurm_cred_t *slurm_cred_faker(slurm_cred_arg_t *arg);

/* Free the credential arguments as loaded by either
 * slurm_cred_get_args() or slurm_cred_verify() */
void slurm_cred_free_args(slurm_cred_arg_t *arg);

/*
 * Release the internal lock acquired through slurm_cred_get_args()
 * or slurm_cred_verify().
 */
extern void slurm_cred_unlock_args(slurm_cred_t *cred);

/*
 * Access the credential's arguments. NULL on error.
 * *Must* release lock with slurm_cred_unlock_args().
 */
extern slurm_cred_arg_t *slurm_cred_get_args(slurm_cred_t *cred);

/*
 * Return a pointer specific field from a job credential
 * cred IN - job credential
 * cred_data_type in - Field desired
 * RET - pointer to the information of interest, NULL on error
 */
extern void *slurm_cred_get(slurm_cred_t *cred,
			    cred_data_enum_t cred_data_type);

/*
 * Return index in rep_count array corresponding to absolute node index
 * cred - job credential to use for memory setting
 * node_name - name of host
 * func_name - name of the calling function (for logging purpose)
 * jot_mem_limit - UPDATED job memory limit
 * step_mem_limit - UPDATED step memory limit
 */
extern void slurm_cred_get_mem(slurm_cred_t *cred,
			      char *node_name,
			      const char *func_name,
			      uint64_t *job_mem_limit,
			      uint64_t *step_mem_limit);
/*
 * Verify the signed credential 'cred', and return cred contents.
 * The credential is cached and cannot be reused.
 *
 * Will perform at least the following checks:
 *   - Credential signature is valid
 *   - Credential has not expired
 *   - If credential is reissue will purge the old credential
 *   - Credential has not been revoked
 *   - Credential has not been replayed
 *
 * *Must* release lock with slurm_cred_unlock_args().
 */
extern slurm_cred_arg_t *slurm_cred_verify(slurm_cred_ctx_t ctx,
					   slurm_cred_t *cred);

/*
 * Rewind the last play of credential cred. This allows the credential
 *  be used again. Returns SLURM_ERROR if no credential state is found
 *  to be rewound, SLURM_SUCCESS otherwise.
 */
int slurm_cred_rewind(slurm_cred_ctx_t ctx, slurm_cred_t *cred);

/*
 * Check to see if this credential is a reissue of an existing credential
 * (this can happen, for instance, with "scontrol restart").  If
 * this credential is a reissue, then the old credential is cleared
 * from the cred context "ctx".
 */
void slurm_cred_handle_reissue(slurm_cred_ctx_t ctx, slurm_cred_t *cred,
			       bool locked);

/*
 * Revoke all credentials for job id jobid
 * time IN - the time the job termination was requested by slurmctld
 *           (local time from slurmctld server)
 * start_time IN - job start time, used to recongnize job requeue
 */
int slurm_cred_revoke(slurm_cred_ctx_t ctx, uint32_t jobid, time_t time,
		      time_t start_time);

/*
 * Report if a all credentials for a give job id have been
 * revoked (i.e. has the job been killed)
 *
 * If we are re-running the job, the new job credential is newer
 * than the revoke time, see "scontrol requeue", purge the old
 * job record and make like it never existed
 */
bool slurm_cred_revoked(slurm_cred_ctx_t ctx, slurm_cred_t *cred);

/*
 * Begin expiration period for the revocation of credentials
 *  for job id jobid. This should be run after slurm_cred_revoke()
 *  This function is used because we may want to revoke credentials
 *  for a jobid, but not purge the revocation from memory until after
 *  some other action has occurred, e.g. completion of a job epilog.
 *
 * Returns 0 for success, SLURM_ERROR for failure with errno set to:
 *
 *  ESRCH  if jobid is not cached
 *  EEXIST if expiration period has already begun for jobid.
 *
 */
int slurm_cred_begin_expiration(slurm_cred_ctx_t ctx, uint32_t jobid);


/*
 * Returns true if the credential context has a cached state for
 * job id jobid.
 */
bool slurm_cred_jobid_cached(slurm_cred_ctx_t ctx, uint32_t jobid);


/*
 * Add a jobid to the slurm credential context without inserting
 * a credential state. This is used by the verifier to track job ids
 * that it has seen, but not necessarily received a credential for.
 */
int slurm_cred_insert_jobid(slurm_cred_ctx_t ctx, uint32_t jobid);

/* Free memory associated with slurm credential `cred.'
 */
void slurm_cred_destroy(slurm_cred_t *cred);

/*
 * Pack a slurm credential for network transmission
 */
void slurm_cred_pack(slurm_cred_t *cred, buf_t *buffer,
		     uint16_t protocol_version);

/*
 * Unpack a slurm job credential
 */
slurm_cred_t *slurm_cred_unpack(buf_t *buffer, uint16_t protocol_version);

/*
 * Get a pointer to the slurm credential signature
 * (used by slurm IO connections to verify connecting agent)
 */
int slurm_cred_get_signature(slurm_cred_t *cred, char **datap,
			     uint32_t *len);

/*
 * Retrieve the set of cores that were allocated to the job and step then
 * format them in the List Format (e.g., "0-2,7,12-14"). Also return
 * job and step's memory limit.
 *
 * NOTE: caller must xfree the returned strings.
 */
void format_core_allocs(slurm_cred_t *cred, char *node_name, uint16_t cpus,
			 char **job_alloc_cores, char **step_alloc_cores,
			 uint64_t *job_mem_limit, uint64_t *step_mem_limit);

/*
 * Retrieve the job and step generic resources (gres) allocate to this job
 * on this node.
 *
 * NOTE: Caller must destroy the returned lists
 */
extern void get_cred_gres(slurm_cred_t *cred, char *node_name,
			  List *job_gres_list, List *step_gres_list);

/*
 * Print a slurm job credential using the info() call
 */
void slurm_cred_print(slurm_cred_t *cred);

/*
 * Functions to create, delete, pack, and unpack an sbcast credential
 * Caller of extract_sbcast_cred() must xfree returned node string
 */

typedef struct {
	uint32_t job_id;
	uint32_t het_job_id;
	uint32_t step_id;
	uid_t uid;
	gid_t gid;
	char *user_name;
	int ngids;
	gid_t *gids;

	time_t expiration;
	char *nodes;
} sbcast_cred_arg_t;

sbcast_cred_t *create_sbcast_cred(slurm_cred_ctx_t ctx,
				  sbcast_cred_arg_t *arg,
				  uint16_t protocol_version);
void delete_sbcast_cred(sbcast_cred_t *sbcast_cred);
sbcast_cred_arg_t *extract_sbcast_cred(slurm_cred_ctx_t ctx,
				       sbcast_cred_t *sbcast_cred,
				       uint16_t block_no,
				       uint16_t flags,
				       uint16_t protocol_version);
void pack_sbcast_cred(sbcast_cred_t *sbcast_cred, buf_t *buffer,
		      uint16_t protocol_Version);
sbcast_cred_t *unpack_sbcast_cred(buf_t *buffer, uint16_t protocol_version);
void print_sbcast_cred(sbcast_cred_t *sbcast_cred);
void sbcast_cred_arg_free(sbcast_cred_arg_t *arg);
extern bool slurm_cred_send_gids_enabled(void);

#endif  /* _HAVE_SLURM_CREDS_H */
