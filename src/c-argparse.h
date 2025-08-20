#ifndef __C_ARGPARSE_H__
#define __C_ARGPARSE_H__

typedef enum {
  CARG_INT32,
  CARG_UINT64,
  CARG_DOUBLE,
  CARG_BOOL,
  CARG_STRING
} carg_type_t;

typedef enum {
  CARG_SUCCESS,
  CARG_INSUFFICIENT_POSITIONALS,
  CARG_UNKNOWN_FLAG,
  CARG_MISSING_VALUE,
  CARG_INVALID_VALUE,
  CARG_UNKNOWN_FAILURE,
  CARG_EXCESSIVE_POSITIONAL
} carg_error_t;

typedef struct {
  const char *long_flag;
  const char short_flag;
  const carg_type_t type;
  void *destination;
} carg_opt_t;

typedef struct {
  const carg_type_t type;
  void *destination;
} carg_pos_t;

const char *carg_strerror(carg_error_t error_level);
carg_error_t carg_parse_args(int argc, char **argv, carg_pos_t *positionals,
                             int num_positionals, carg_opt_t *options,
                             int num_options);
void carg_print_args(carg_pos_t *positionals, int num_positionals,
                     carg_opt_t *options, int num_options);
#endif // !__C_ARGPARSE_H__
