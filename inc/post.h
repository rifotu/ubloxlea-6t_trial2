
#ifndef __post_h__
#define __post_h__

typedef struct post_s
{
    char *postAddress;
    char *postData;
}post_t;


// function prototypes
int post_initialize(void);
int post2Server(struct post_s *post_p);
int postCleanup(void);

#endif
