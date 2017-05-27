#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mecab.h>
#include "emacs-module.h"

int plugin_is_GPL_compatible;

/* emacs lisp symbols */
static emacs_value Qnil;
static emacs_value Qt;
static emacs_value Fcons;
static emacs_value Fboundp;
static emacs_value Fsymbol_value;
static emacs_value Fstringp;
static emacs_value Fmessage;
static emacs_value Fjapanese_hiragana;

static emacs_value Qmecab_rcfile;

/* intern lisp symbols */
static void
intern_lisp_symbols (emacs_env *env)
{
  Qnil = env->intern (env, "nil");
  Qt = env->intern (env, "t");
  Fcons = env->intern (env, "cons");
  Fboundp = env->intern (env, "boundp");
  Fsymbol_value = env->intern (env, "symbol-value");
  Fstringp = env->intern (env, "stringp");
  Fmessage = env->intern (env, "message");
  Fjapanese_hiragana = env->intern (env, "japanese-hiragana");

  Qmecab_rcfile = env->intern (env, "mecab-rcfile");
}

static emacs_value
cons (emacs_env *env, emacs_value car, emacs_value cdr)
{
  emacs_value args[] = { car, cdr };
  return env->funcall (env, Fcons, 2, args);
}
static bool
boundp (emacs_env *env, emacs_value symbol)
{
  emacs_value args[] = { symbol };
  return env->eq (env, env->funcall (env, Fboundp, 1, args), Qt);
}
static emacs_value
symbol_value (emacs_env *env, emacs_value symbol)
{
  emacs_value args[] = { symbol };
  return env->funcall (env, Fsymbol_value, 1, args);
}
static bool
stringp (emacs_env *env, emacs_value obj)
{
  emacs_value args[] = { obj };
  return env->eq (env, env->funcall (env, Fstringp, 1, args), Qt);
}
static emacs_value
hiragana (emacs_env *env, const char *str)
{
  emacs_value args[] = { env->make_string (env, str, strlen (str)) };
  return env->funcall (env, Fjapanese_hiragana, 1, args);
}

extern int vasprintf(char **, const char *, va_list);

static emacs_value
message (emacs_env *env, const char *fmt, ...)
{
  va_list arg;
  va_start (arg, fmt);
  char *buf = NULL;

  if (vasprintf (&buf, fmt, arg) == -1) {
    va_end (arg);
    return Qnil;
  }
  emacs_value args[] = { env->make_string (env, buf, strlen (buf)) };
  free (buf);
  va_end (arg);
  return env->funcall (env, Fmessage, 1, args);
}

static mecab_t *mecab = NULL;

static void
mecab_init (emacs_env *env)
{
  emacs_value mecab_rcfile;
  ptrdiff_t size;
  char *rcfile = NULL;
  ptrdiff_t arg_size;
  char *arg;

  if (mecab) return;

  arg_size = 38 /* strlen ("--node-format=%pS%f[7] --unk-format=%M") */ ;
  if (boundp (env, Qmecab_rcfile)) {
    mecab_rcfile = symbol_value (env, Qmecab_rcfile);
    if (stringp (env, mecab_rcfile)) {
      env->copy_string_contents (env, mecab_rcfile, rcfile, &size);
      rcfile = alloca (10 /* strlen (" --rcfile=") */ + size);
      strcpy (rcfile, " --rcfile=");
      env->copy_string_contents (env, mecab_rcfile, &rcfile[10], &size);
      arg_size += strlen (rcfile);
    }
  }
  arg = alloca (arg_size + 1);
  snprintf (arg, arg_size + 1,
    "--node-format=%%pS%%f[7] --unk-format=%%M%s", rcfile ? rcfile : "");

  mecab = mecab_new2 (arg);
}

static emacs_value
Freverse_translate_driver_mecab_module_terminate (emacs_env *env,
                                                  ptrdiff_t nargs,
						  emacs_value args[],
						  void *data)
{
  if (mecab) mecab_destroy (mecab);
  mecab = NULL;
  return Qnil;
}

static emacs_value
Freverse_translate_driver_mecab_module_initialize (emacs_env *env,
                                                   ptrdiff_t nargs,
                                                   emacs_value args[],
						   void *data)
{
  Freverse_translate_driver_mecab_module_terminate (env, nargs, args, data);
  mecab_init (env);
  return Qnil;
}

static emacs_value
make_response (emacs_env *env, const mecab_node_t *node)
{
  if (node) {
    switch (node->stat) {
    case MECAB_BOS_NODE:
      return make_response (env, node->next);
      break;
    case MECAB_NOR_NODE:
    case MECAB_UNK_NODE:
      {
        char *buf = NULL;

        buf = alloca (node->length + 1);
        memcpy (buf, node->surface, node->length);
        buf[node->length] = '\0';

        return cons (env,
                     cons (env,
                           env->make_string (env, buf, strlen (buf)),
                           hiragana (env, mecab_format_node (mecab, node))),
                     make_response (env, node->next));
      }
      break;
    default:
      return Qnil;
    }
  }
  return Qnil;
}

static emacs_value
Freverse_translate_driver_mecab_module (emacs_env *env, ptrdiff_t nargs,
                                        emacs_value args[], void *data)
{
  emacs_value lisp_str = args[0];
  ptrdiff_t size = 0;
  char *kanji = NULL;
  const mecab_node_t *node;

  env->copy_string_contents (env, lisp_str, kanji, &size);
  kanji = alloca (size);
  env->copy_string_contents (env, lisp_str, kanji, &size);

  mecab_init (env);
  if (mecab == NULL) {
    message (env, "can not initialize mecab module.");
    return Qnil;
  }
  node = mecab_sparse_tonode (mecab, kanji);
  return make_response (env, node);
}

/* Lisp utilities for easier readability (simple wrappers).  */

/* Provide FEATURE to Emacs.  */
static void
provide (emacs_env *env, const char *feature)
{
  emacs_value Qfeat = env->intern (env, feature);
  emacs_value Qprovide = env->intern (env, "provide");
  emacs_value args[] = { Qfeat };

  env->funcall (env, Qprovide, 1, args);
}

/* Bind NAME to FUN.  */
static void
bind_function (emacs_env *env, const char *name, emacs_value Sfun)
{
  emacs_value Qfset = env->intern (env, "fset");
  emacs_value Qsym = env->intern (env, name);
  emacs_value args[] = { Qsym, Sfun };

  env->funcall (env, Qfset, 2, args);
}

/* Module init function.  */
int
emacs_module_init (struct emacs_runtime *ert)
{
  emacs_env *env = ert->get_environment (ert);

  intern_lisp_symbols (env);

#define DEFUN(lsym, csym, amin, amax, doc, data) \
  bind_function (env, lsym, \
		 env->make_function (env, amin, amax, csym, doc, data))

  DEFUN ("dcj:reverse-translate-driver-mecab-module-initialize",
    Freverse_translate_driver_mecab_module_initialize, 0, 0,
    "Initialize dynamic module version reverse-translate-driver-mecab.", NULL);
  DEFUN ("dcj:reverse-translate-driver-mecab-module-terminate",
    Freverse_translate_driver_mecab_module_terminate, 0, 0,
    "Terminate dynamic module version reverse-translate-driver-mecab.", NULL);
  DEFUN ("dcj:reverse-translate-driver-mecab-module",
    Freverse_translate_driver_mecab_module, 1, 1,
    "Dynamic module version reverse-translate-driver-mecab.", NULL);

#undef DEFUN

  provide (env, "reverse-translate-driver-mecab-module");
  return 0;
}
