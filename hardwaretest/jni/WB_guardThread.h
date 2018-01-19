#ifndef   _WB_GUARD_THREAD_H__
#define   _WB_GUARD_THREAD_H__





typedef struct GuardThreadOps{
	int (*setGuardPackagenameAndMainclassname)(struct GuardThreadOps* ,const char *,const char * ,int);
}GuardThreadOps,*pGuardThreadOps;

pGuardThreadOps  createGuardThreadServer(void);
void destroyGuardThreadServer(pGuardThreadOps *);











#endif
