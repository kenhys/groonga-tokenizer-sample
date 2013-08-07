#include <groonga/tokenizer.h>

#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <wn.h>

#ifdef __GNUC__
#  define GNUC_UNUSED __attribute__((__unused__))
#else
#  define GNUC_UNUSED
#endif

typedef struct {
  const char *next;
  const char *end;
  int rest;
  grn_tokenizer_query *query;
  grn_tokenizer_token token;
} wordnet_tokenizer;


static grn_obj *
wordnet_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  wordnet_tokenizer *tokenizer;
  unsigned int normalizer_flags = 0;
  grn_tokenizer_query *query;
  grn_obj *normalized_query;
  const char *normalized_string;
  unsigned int normalized_string_length;

  if (wninit() == -1) {
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[tokenizer][wordnet] "
                     "failed to initialize wordnet library");
    return NULL;
  }

  query = grn_tokenizer_query_open(ctx, nargs, args, normalizer_flags);
  if (!query) {
    return NULL;
  }

  tokenizer = GRN_PLUGIN_MALLOC(ctx, sizeof(wordnet_tokenizer));
  if (!tokenizer) {
    grn_tokenizer_query_close(ctx, query);
    GRN_PLUGIN_ERROR(ctx, GRN_NO_MEMORY_AVAILABLE,
                     "[tokenizer][wordnet] "
                     "memory allocation to wordnet_tokenizer failed");
    return NULL;
  }

  tokenizer->query = query;

  normalized_query = query->normalized_query;
  grn_string_get_normalized(ctx,
                            normalized_query,
                            &normalized_string,
                            &normalized_string_length,
                            NULL);
  tokenizer->next = normalized_string;
  tokenizer->rest = normalized_string_length;
  tokenizer->end = tokenizer->next + tokenizer->rest;

  user_data->ptr = tokenizer;

  grn_tokenizer_token_init(ctx, &(tokenizer->token));

  return NULL;
}

static void wordnet_push_token(grn_ctx *ctx, wordnet_tokenizer *tokenizer,
                               const char *token, int pending, grn_tokenizer_status status)
{
  int pos;
  char *morph;
  int actual_length;

  actual_length = pending;
  for (pos = 5; pos >= 1; pos--) {
    *(char *)token = '\0';
    morph = morphstr((char *)token - pending, pos);
    if (morph) {
      actual_length = strlen(morph);
      memset((void *)token - pending, '=', pending);
      memcpy((void *)token - pending, morph, actual_length);
      break;
    }
  }
  grn_tokenizer_token_push(ctx, &(tokenizer->token),
                           token - pending,
                           actual_length, status);
}

static grn_obj *
wordnet_next(grn_ctx *ctx, GNUC_UNUSED int nargs, GNUC_UNUSED grn_obj **args,
            grn_user_data *user_data)
{
  wordnet_tokenizer *tokenizer = user_data->ptr;
  grn_encoding encoding = tokenizer->query->encoding;
  grn_tokenizer_status status;
  const char *token;
  int token_length;
  int spaces;
  int consumed;
  int pending;

  consumed = 0;
  pending = 0;
  for (token = tokenizer->next;
       token < tokenizer->end;
       token += token_length) {
    token_length = grn_plugin_charlen(ctx, token, tokenizer->rest, encoding);
    spaces = grn_plugin_isspace(ctx, token, tokenizer->rest, encoding);
    if (spaces > 0) {
      if (pending > 0) {
        if (token + spaces >= tokenizer->end) {
          status = GRN_TOKENIZER_LAST;
        } else {
          status = GRN_TOKENIZER_CONTINUE;
        }
        wordnet_push_token(ctx, tokenizer, token, pending, status);
      }
      pending = 0;
      token_length = spaces;
      consumed += spaces;
      break;
    } else {
      pending += token_length;
      consumed += token_length;
    }
  }
  if (pending > 0) {
    wordnet_push_token(ctx, tokenizer, token, pending, GRN_TOKENIZER_LAST);
  }
  tokenizer->next += consumed;
  tokenizer->rest -= consumed;

  return NULL;
}

static grn_obj *
wordnet_fin(grn_ctx *ctx, GNUC_UNUSED int nargs, GNUC_UNUSED grn_obj **args,
           grn_user_data *user_data)
{
  wordnet_tokenizer *tokenizer = user_data->ptr;

  if (!tokenizer) {
    return NULL;
  }

  grn_tokenizer_token_fin(ctx, &(tokenizer->token));
  grn_tokenizer_query_close(ctx, tokenizer->query);
  GRN_PLUGIN_FREE(ctx, tokenizer);

  return NULL;
}


grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  return ctx->rc;
}

grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_rc rc;

  rc = grn_tokenizer_register(ctx, "TokenWordNet", -1,
                              wordnet_init, wordnet_next, wordnet_fin);

  return rc;
}

grn_rc
GRN_PLUGIN_FIN(GNUC_UNUSED grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
