#include "c-argparse.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_10 (10)
typedef enum {
  POSITIONAL,
  SHORT_FLAG,
  LONG_FLAG,
  OPTIONS_STOP
} detected_flag_t;

static carg_error_t arg_to_int(const char *arg, void *dest);
static carg_error_t arg_to_unsigned_long(const char *arg, void *dest);
static carg_error_t arg_to_double(const char *arg, void *dest);
static carg_error_t arg_to_string(const char *arg, void *dest);
static carg_error_t set_arg(const char *arg, void *dest, carg_type_t arg_type);

static carg_error_t identify_arg_type(const char *arg,
                                      detected_flag_t *detected_type);
static carg_error_t handle_positional(const char *arg, int positional_ctr,
                                      carg_pos_t *positionals,
                                      int num_positionals);
static carg_error_t handle_short_flag(char **argv, int *arg_idx_ptr,
                                      carg_opt_t *options, int num_options);

static carg_error_t handle_long_flag(char **argv, int *arg_idx_ptr,
                                     carg_opt_t *options, int num_options);

/* -- Implementation -------------------------------------------------------- */
const char *carg_strerror(carg_error_t error_level) {
  switch (error_level) {
  case CARG_SUCCESS:
    return "CARG_SUCCESS";
  case CARG_MISSING_VALUE:
    return "CARG_MISSING_VALUE";
  case CARG_INSUFFICIENT_POSITIONALS:
    return "CARG_INSUFFICIENT_ARGS";
  case CARG_UNKNOWN_FLAG:
    return "CARG_UNKNOWN_ARG";
  case CARG_UNKNOWN_FAILURE:
    return "CARG_UNKNOWN_FAILURE";
  case CARG_EXCESSIVE_POSITIONAL:
    return "CARG_EXCESSIVE_POSITIONAL";
  case CARG_INVALID_VALUE:
    return "CARG_INVALID_VALUE";
  default:
    return "";
  }
}
carg_error_t carg_parse_args(const int argc, char **argv,
                             carg_pos_t *positionals, int num_positionals,
                             carg_opt_t *options, int num_options) {
  carg_error_t error_level = CARG_SUCCESS;
  int positional_counter   = 0;
  int stop_options         = 0;
  if (argc - 1 < num_positionals) {
    error_level = CARG_INSUFFICIENT_POSITIONALS;
    goto early_exit;
  }

  for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
    detected_flag_t arg_type;
    error_level = identify_arg_type(argv[arg_idx], &arg_type);
    if (arg_type == OPTIONS_STOP) {
      stop_options = 1;
      continue;
    }
    if (arg_type == POSITIONAL || stop_options) {
      error_level = handle_positional(argv[arg_idx], positional_counter,
                                      positionals, num_positionals);
      positional_counter++;
    } else if (arg_type == SHORT_FLAG) {
      error_level = handle_short_flag(argv, &arg_idx, options, num_options);
    } else if (arg_type == LONG_FLAG) {
      error_level = handle_long_flag(argv, &arg_idx, options, num_options);
    }

    if (error_level != CARG_SUCCESS)
      goto early_exit;
  }
  if (positional_counter != num_positionals)
    error_level = CARG_INSUFFICIENT_POSITIONALS;
early_exit:
  return error_level;
}

void carg_print_args(carg_pos_t *positionals, int num_positionals,
                     carg_opt_t *options, int num_options) {
  for (int pos_idx = 0; pos_idx < num_positionals; pos_idx++) {
    const int format_pattern_len = 10;
    char format_pattern[]        = "pos_%d : %0 \n";
    switch (positionals[pos_idx].type) {
    case CARG_INT32:
      format_pattern[format_pattern_len] = 'd';
      printf(format_pattern, pos_idx,
             *((int32_t *)positionals[pos_idx].destination));
      break;
    case CARG_UINT64:
      format_pattern[format_pattern_len]     = 'l';
      format_pattern[format_pattern_len + 1] = 'u';
      printf(format_pattern, pos_idx,
             *((uint64_t *)positionals[pos_idx].destination));
      break;
    case CARG_DOUBLE:
      format_pattern[format_pattern_len] = 'e';
      printf(format_pattern, pos_idx,
             *((double *)(positionals[pos_idx].destination)));
      break;
    case CARG_BOOL:
      format_pattern[format_pattern_len] = 'd';
      printf(format_pattern, pos_idx,
             *((int *)positionals[pos_idx].destination));
      break;
    case CARG_STRING:
      format_pattern[format_pattern_len] = 's';
      printf(format_pattern, pos_idx,
             *(const char **)positionals[pos_idx].destination);
      break;
    }
  }
  for (int opt_idx = 0; opt_idx < num_options; opt_idx++) {
    const int format_pattern_len = 10;
    char format_pattern[]        = "%s (%c): %0 \n";
    switch (options[opt_idx].type) {
    case CARG_INT32:
      format_pattern[format_pattern_len] = 'd';
      printf(format_pattern, options[opt_idx].long_flag,
             options[opt_idx].short_flag,
             *((int *)options[opt_idx].destination));
      break;
    case CARG_UINT64:
      format_pattern[format_pattern_len]     = 'l';
      format_pattern[format_pattern_len + 1] = 'u';
      printf(format_pattern, options[opt_idx].long_flag,
             options[opt_idx].short_flag,
             *((unsigned long *)options[opt_idx].destination));
      break;
    case CARG_DOUBLE:
      format_pattern[format_pattern_len] = 'e';
      printf(format_pattern, options[opt_idx].long_flag,
             options[opt_idx].short_flag,
             *((double *)options[opt_idx].destination));
      break;
    case CARG_BOOL:
      format_pattern[format_pattern_len] = 'd';
      printf(format_pattern, options[opt_idx].long_flag,
             options[opt_idx].short_flag,
             *((int *)options[opt_idx].destination));
      break;
    case CARG_STRING:
      format_pattern[format_pattern_len] = 's';
      printf(format_pattern, options[opt_idx].long_flag,
             options[opt_idx].short_flag,
             *(const char **)options[opt_idx].destination);
      break;
    }
  }
}

static carg_error_t identify_arg_type(const char *const arg,
                                      detected_flag_t *detected_type) {

  detected_flag_t flag_type = POSITIONAL;
  carg_error_t error_level  = CARG_SUCCESS;
  size_t arg_len            = strlen(arg);

  if (arg_len == 0) {
    error_level = CARG_UNKNOWN_FAILURE;
    goto early_exit;
  }

  if (arg_len > 1 && arg[0] == '-')
    flag_type = SHORT_FLAG;
  if (arg_len > 2 && arg[0] == '-' && arg[1] == '-')
    flag_type = LONG_FLAG;
  if (arg_len == 2 && arg[0] == '-' && arg[1] == '-')
    flag_type = OPTIONS_STOP;

  *detected_type = flag_type;
early_exit:
  return error_level;
}

static carg_error_t handle_positional(const char *arg, int positional_ctr,
                                      carg_pos_t *positionals,
                                      int num_positionals) {
  carg_error_t error_level = CARG_SUCCESS;
  if (!(positional_ctr < num_positionals)) {
    error_level = CARG_EXCESSIVE_POSITIONAL;
    goto early_exit;
  }
  error_level = set_arg(arg, positionals[positional_ctr].destination,
                        positionals[positional_ctr].type);
early_exit:
  return error_level;
}

static carg_error_t handle_short_flag(char **argv, int *arg_idx_ptr,
                                      carg_opt_t *options, int num_options) {
  const int arg_idx        = *arg_idx_ptr;
  carg_error_t error_level = CARG_SUCCESS;
  char arg_flag            = argv[arg_idx][1];
  size_t arg_len           = strlen(&argv[arg_idx][1]);
  char found               = 0;
  int opt_idx              = 0;
  char *arg_value          = 0;
  for (opt_idx = 0; opt_idx < num_options; opt_idx++) {
    if (options[opt_idx].short_flag == arg_flag) {
      found = 1;
      break;
    }
  }
  if (!found) {
    error_level = CARG_UNKNOWN_FLAG;
    goto early_exit;
  }
  if (options[opt_idx].type == CARG_BOOL && arg_len > 1) {
    error_level = CARG_UNKNOWN_FLAG;
    goto early_exit;
  }
  if (arg_len == 1 && options[opt_idx].type != CARG_BOOL &&
      argv[arg_idx + 1] == NULL) {
    error_level = CARG_MISSING_VALUE;
    goto early_exit;
  }
  if (arg_len > 1) {
    arg_value = &argv[arg_idx][2];
  } else if (options[opt_idx].type != CARG_BOOL) {
    arg_value = argv[arg_idx + 1];
    (*arg_idx_ptr)++;
  }
  error_level =
      set_arg(arg_value, options[opt_idx].destination, options[opt_idx].type);
early_exit:
  return error_level;
}

static carg_error_t handle_long_flag(char **argv, int *arg_idx_ptr,
                                     carg_opt_t *options, int num_options) {
  const int arg_idx        = *arg_idx_ptr;
  carg_error_t error_level = CARG_SUCCESS;
  char *arg_flag           = &argv[arg_idx][2];
  size_t arg_len           = strlen(&argv[arg_idx][2]);
  size_t def_len           = 0;
  char found               = 0;
  int opt_idx              = 0;
  char *arg_value          = 0;

  for (opt_idx = 0; opt_idx < num_options; opt_idx++) {
    def_len = strlen(options[opt_idx].long_flag);

    if (arg_len == def_len &&
        strncmp(options[opt_idx].long_flag, arg_flag, def_len) == 0) {
      found = 1;
      break;
    }
    if (arg_len > def_len &&
        strncmp(options[opt_idx].long_flag, arg_flag, def_len) == 0 &&
        arg_flag[def_len] == '=') {
      found = 1;
      break;
    }
  }
  if (!found) {
    error_level = CARG_UNKNOWN_FLAG;
    goto early_exit;
  }
  if (options[opt_idx].type == CARG_BOOL && arg_len > def_len) {
    error_level = CARG_UNKNOWN_FLAG;
    goto early_exit;
  }
  if (arg_len == def_len && options[opt_idx].type != CARG_BOOL &&
      argv[arg_idx + 1] == NULL) {
    error_level = CARG_MISSING_VALUE;
    goto early_exit;
  }
  if (arg_len > def_len) {
    arg_value = &arg_flag[def_len + 1];
  } else if (options[opt_idx].type != CARG_BOOL) {
    arg_value = argv[arg_idx + 1];
    (*arg_idx_ptr)++;
  }
  error_level =
      set_arg(arg_value, options[opt_idx].destination, options[opt_idx].type);
early_exit:
  return error_level;
}
static carg_error_t arg_to_int(const char *arg, void *dest) {

  carg_error_t error_level = CARG_SUCCESS;
  char *num_end            = NULL;
  long buffer              = 0;
  if (!arg || !*arg) {
    error_level = CARG_UNKNOWN_FAILURE;
    goto early_exit;
  }
  errno  = 0;
  buffer = strtol(arg, &num_end, BASE_10);
  if (*num_end != '\0' || errno == ERANGE) {
    error_level = CARG_INVALID_VALUE;
    goto early_exit;
  }
  if (buffer > INT32_MAX || buffer < INT32_MIN) {
    error_level = CARG_INVALID_VALUE;
    goto early_exit;
  } else
    *(int32_t *)dest = (int32_t)buffer;
early_exit:
  return error_level;
}

static carg_error_t arg_to_unsigned_long(const char *arg, void *dest) {
  carg_error_t error_level  = CARG_SUCCESS;
  char *num_end             = NULL;
  unsigned long long buffer = 0;
  if (!arg || !*arg) {
    error_level = CARG_UNKNOWN_FAILURE;
    goto early_exit;
  }
  if (arg[0] == '-') {
    error_level = CARG_INVALID_VALUE;
    goto early_exit;
  }
  errno  = 0;
  buffer = strtoull(arg, &num_end, BASE_10);
  if (*num_end != '\0' || errno == ERANGE) {
    error_level = CARG_INVALID_VALUE;
    goto early_exit;
  }
  if (buffer > UINT64_MAX) {
    error_level = CARG_INVALID_VALUE;
    goto early_exit;
  } else
    *(uint64_t *)dest = (uint64_t)buffer;
early_exit:
  return error_level;
}

static carg_error_t arg_to_double(const char *arg, void *dest) {
  carg_error_t error_level = CARG_SUCCESS;
  double buffer            = 0;
  char *num_end            = NULL;

  if (!arg || !*arg) {
    error_level = CARG_UNKNOWN_FAILURE;
    goto early_exit;
  }
  errno  = 0;
  buffer = strtod(arg, &num_end);
  if (*num_end != '\0' || errno == ERANGE) {
    error_level = CARG_INVALID_VALUE;
    goto early_exit;
  }
  *(double *)dest = buffer;
early_exit:
  return error_level;
}

static carg_error_t arg_to_string(const char *arg, void *dest) {
  carg_error_t error_level = CARG_SUCCESS;
  if (!arg || !*arg) {
    error_level = CARG_UNKNOWN_FAILURE;
    goto early_exit;
  }
  *(const char **)dest = arg;
early_exit:
  return error_level;
}

static carg_error_t set_arg(const char *arg, void *dest, carg_type_t arg_type) {
  carg_error_t error_level = CARG_SUCCESS;
  switch (arg_type) {
  case CARG_INT32:
    error_level = arg_to_int(arg, dest);
    break;
  case CARG_UINT64:
    error_level = arg_to_unsigned_long(arg, dest);
    break;
  case CARG_DOUBLE:
    error_level = arg_to_double(arg, dest);
    break;
  case CARG_BOOL:
    *(int *)dest = 1;
    break;
  case CARG_STRING:
    error_level = arg_to_string(arg, dest);
    break;
  }
  return error_level;
}
