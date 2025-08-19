#include "c-argparse.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LONG_FLAG_LENGHT (256)
#define BASE_10 (10)

typedef enum { POSITIONAL, SHORT_FLAG, LONG_FLAG } detected_flag_t;

static void arg_to_int(char *arg, void *dest);
static void arg_to_unsigned_long(char *arg, void *dest);
static void arg_to_float(char *arg, void *dest);
static void arg_to_string(char *arg, void *dest);
static void set_arg(char *arg, void *dest, carg_type_t arg_type);
static char *detected_flag_to_str(detected_flag_t flag);
static carg_error_t identify_arg_type(const char *arg,
                                      detected_flag_t *detected_type);
static carg_error_t handle_positional(const char *arg, int positional_ctr,
                                      carg_pos_t *positionals,
                                      int num_positionals);

static carg_error_t handle_short_flag(char **argv, int *arg_idx_ptr,
                                      carg_opt_t *options, int num_options);
/* -- Implementation -------------------------------------------------------- */

carg_error_t carg_parse_args(const int argc, char **argv,
                             carg_pos_t *positionals, int num_positionals,
                             carg_opt_t *options, int num_options) {
  carg_error_t error_level = CARG_SUCCESS;
  int positional_counter   = 0;

  if (argc - 1 < num_positionals) {
    error_level = CARG_INSUFFICIENT_ARGS;
    goto early_exit;
  }
  for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
    detected_flag_t arg_type;
    identify_arg_type(argv[arg_idx], &arg_type);
    if (arg_type == POSITIONAL) {

      error_level = handle_positional(argv[arg_idx], positional_counter,
                                      positionals, num_positionals);
      positional_counter++;
    }
    if (arg_type == SHORT_FLAG) {
      error_level = handle_short_flag(argv, &arg_idx, options, num_options);
    }
    if (error_level != CARG_SUCCESS)
      goto early_exit;
    printf("%d: %s %s\n", arg_idx, argv[arg_idx],
           detected_flag_to_str(arg_type));
  }
  if (positional_counter != num_positionals)
    error_level = CARG_INSUFFICIENT_ARGS;
early_exit:
  return error_level;
}

void carg_print_args(carg_pos_t *positionals, int num_positionals,
                     carg_opt_t *options, int num_options) {
  for (int pos_idx = 0; pos_idx < num_positionals; pos_idx++) {
    const int format_pattern_len = 10;
    char format_pattern[]        = "pos_%d : %0 \n";
    switch (positionals[pos_idx].type) {
    case CARG_INT:
      format_pattern[format_pattern_len] = 'd';
      printf(format_pattern, pos_idx,
             *((int *)positionals[pos_idx].destination));
      break;
    case CARG_UNSIGNED_LONG:
      format_pattern[format_pattern_len]     = 'l';
      format_pattern[format_pattern_len + 1] = 'u';
      printf(format_pattern, pos_idx,
             *((unsigned long *)positionals[pos_idx].destination));
      break;
    case CARG_FLOAT:
      format_pattern[format_pattern_len] = 'e';

      printf(format_pattern, pos_idx,
             *((double *)positionals[pos_idx].destination));
      break;
    case CARG_BOOL:
      format_pattern[format_pattern_len] = 'd';
      printf(format_pattern, pos_idx,
             *((int *)positionals[pos_idx].destination));
      break;
    case CARG_STRING:
      format_pattern[format_pattern_len] = 's';
      printf(format_pattern, pos_idx, (char *)positionals[pos_idx].destination);
      break;
    }
  }
  for (int opt_idx = 0; opt_idx < num_options; opt_idx++) {
    const int format_pattern_len = 10;
    char format_pattern[]        = "%s (%c): %0 \n";
    switch (options[opt_idx].type) {
    case CARG_INT:
      format_pattern[format_pattern_len] = 'd';
      printf(format_pattern, options[opt_idx].long_flag,
             options[opt_idx].short_flag,
             *((int *)options[opt_idx].destination));
      break;
    case CARG_UNSIGNED_LONG:
      format_pattern[format_pattern_len]     = 'l';
      format_pattern[format_pattern_len + 1] = 'u';
      printf(format_pattern, options[opt_idx].long_flag,
             options[opt_idx].short_flag,
             *((unsigned long *)options[opt_idx].destination));
      break;
    case CARG_FLOAT:
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
             options[opt_idx].short_flag, (char *)options[opt_idx].destination);
      break;
    }
  }
}

static carg_error_t identify_arg_type(const char *const arg,
                                      detected_flag_t *detected_type) {

  detected_flag_t flag_type = POSITIONAL;
  carg_error_t error_level  = CARG_SUCCESS;
  size_t arg_len            = strnlen(arg, MAX_LONG_FLAG_LENGHT);
  if (arg_len == 0) {
    error_level = CARG_UNKNOWN_FAILURE;
    goto early_exit;
  }
  if (arg_len > 1 && arg[0] == '-') {
    flag_type = SHORT_FLAG;
  }
  if (arg_len > 2 && arg[0] == '-' && arg[1] == '-') {
    flag_type = LONG_FLAG;
  }
  *detected_type = flag_type;
early_exit:
  return error_level;
}

static char *detected_flag_to_str(detected_flag_t flag) {
  switch (flag) {
  case POSITIONAL:
    return "POSITIONAL";
  case SHORT_FLAG:
    return "SHORT_FLAG";
  case LONG_FLAG:
    return "LONG_FLAG";
  default:
    return "UNKNOWN";
  }
}

static carg_error_t handle_positional(const char *arg, int positional_ctr,
                                      carg_pos_t *positionals,
                                      int num_positionals) {
  carg_error_t error_level = CARG_SUCCESS;
  if (!(positional_ctr < num_positionals)) {
    error_level = CARG_EXCESSIVE_POSITIONAL;
    goto early_exit;
  }
  switch (positionals[positional_ctr].type) {
  case CARG_INT:
    *(int *)positionals[positional_ctr].destination = atoi(arg);
    break;
  case CARG_UNSIGNED_LONG:
    *(unsigned long *)positionals[positional_ctr].destination =
        strtoul(arg, NULL, BASE_10);
    break;
  case CARG_FLOAT:
    *(double *)positionals[positional_ctr].destination = atof(arg);
    break;
  case CARG_BOOL:
    // SHOULD NOT HAVE TYPE BOOL
    break;
  case CARG_STRING:
    positionals[positional_ctr].destination = (void *)arg;
    break;
  }
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
    error_level = CARG_UNKNOWN_ARG;
    goto early_exit;
  }
  if (options[opt_idx].type == CARG_BOOL) {
    arg_value = NULL;
  } else if (arg_len > 1) {
    arg_value = &argv[arg_idx][2];
  } else {
    arg_value = argv[arg_idx + 1];
    (*arg_idx_ptr)++;
  }
  switch (options[opt_idx].type) {
  case CARG_INT:
    *(int *)options[opt_idx].destination = atoi(arg_value);
    break;
  case CARG_UNSIGNED_LONG:
    *(unsigned long *)options[opt_idx].destination =
        strtoul(arg_value, NULL, BASE_10);
    break;
  case CARG_FLOAT:
    *(double *)options[opt_idx].destination = atof(arg_value);
    break;
  case CARG_BOOL:
    *(int *)options[opt_idx].destination = 1;
    break;
  case CARG_STRING:
    options[opt_idx].destination = (void *)arg_value;
    break;
  }
early_exit:
  return error_level;
}
