/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "finder.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"           /* ONNX Runtime's own header, not ours */
#include <onnxruntime_c_api.h>
#pragma GCC diagnostic pop
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct EaselFinder
{
  const OrtApi *api;
  OrtEnv *env;
  OrtSession *session;
  OrtRunOptions *run;               /* one per finder: easel_finder_cancel marks it, a new run clears it */
  OrtMemoryInfo *memory;
  char *input_name, *output_name;
  int side;
  float *normalized;                /* the model's own input: (x - 0.5) / 1.0, IS-Net's normalization (ADR-0006) */
  int cancelled;                    /* set by easel_finder_cancel (any thread), cleared by a new run */
};

/* The status's message copied for the caller, and the status released; NULL for no status. */
static char *
message_of (const OrtApi *api, OrtStatus *status)
{
  char *m;

  if (status == NULL)
    return NULL;
  m = strdup (api->GetErrorMessage (status));
  api->ReleaseStatus (status);
  return m != NULL ? m : strdup ("ONNX Runtime error");
}

static void
release (EaselFinder *f)
{
  const OrtApi *api = f->api;
  OrtAllocator *al = NULL;

  if (api->GetAllocatorWithDefaultOptions (&al) == NULL)
    {
      OrtStatus *s;

      if (f->input_name != NULL && (s = api->AllocatorFree (al, f->input_name)) != NULL)
        api->ReleaseStatus (s);
      if (f->output_name != NULL && (s = api->AllocatorFree (al, f->output_name)) != NULL)
        api->ReleaseStatus (s);
    }
  if (f->memory != NULL)
    api->ReleaseMemoryInfo (f->memory);
  if (f->run != NULL)
    api->ReleaseRunOptions (f->run);
  if (f->session != NULL)
    api->ReleaseSession (f->session);
  if (f->env != NULL)
    api->ReleaseEnv (f->env);
  free (f->normalized);
  free (f);
}

EaselFinder *
easel_finder_open (const char *path, int threads, char **error)
{
  const OrtApiBase *base = OrtGetApiBase ();
  EaselFinder *f;
  OrtSessionOptions *options = NULL;
  OrtAllocator *al = NULL;
  OrtTypeInfo *type = NULL;
  const OrtTensorTypeAndShapeInfo *tensor = NULL;
  int64_t dims[4] = { 0 };
  size_t n_dims = 0;
  char *m = NULL;

  if (error != NULL)
    *error = NULL;
  f = calloc (1, sizeof *f);
  if (f == NULL)
    goto fail;
  f->api = base->GetApi (ORT_API_VERSION);
  if (f->api == NULL)
    {
      m = strdup ("this ONNX Runtime is older than the one Easel was built with");
      free (f);
      f = NULL;
      goto fail;
    }
#define TRY(call) do { if ((m = message_of (f->api, (call))) != NULL) goto fail; } while (0)
  TRY (f->api->CreateEnv (ORT_LOGGING_LEVEL_ERROR, "easel", &f->env));
  TRY (f->api->CreateSessionOptions (&options));
  TRY (f->api->SetIntraOpNumThreads (options, threads > 0 ? threads : 1));
  TRY (f->api->SetInterOpNumThreads (options, 1));
  TRY (f->api->CreateSession (f->env, path, options, &f->session));
  TRY (f->api->GetAllocatorWithDefaultOptions (&al));
  TRY (f->api->SessionGetInputName (f->session, 0, al, &f->input_name));
  TRY (f->api->SessionGetOutputName (f->session, 0, al, &f->output_name));
  TRY (f->api->SessionGetInputTypeInfo (f->session, 0, &type));
  TRY (f->api->CastTypeInfoToTensorInfo (type, &tensor));
  TRY (f->api->GetDimensionsCount (tensor, &n_dims));
  if (n_dims != 4)
    {
      m = strdup ("the model's input is not 1 x 3 x side x side");
      goto fail;
    }
  TRY (f->api->GetDimensions (tensor, dims, 4));
  if (dims[1] != 3 || dims[2] <= 0 || dims[2] != dims[3] || dims[2] > 8192)
    {
      m = strdup ("the model's input is not 1 x 3 x side x side");
      goto fail;
    }
  f->side = (int) dims[2];
  TRY (f->api->CreateRunOptions (&f->run));
  TRY (f->api->CreateCpuMemoryInfo (OrtArenaAllocator, OrtMemTypeDefault, &f->memory));
#undef TRY
  f->normalized = malloc ((size_t) 3 * f->side * f->side * sizeof *f->normalized);
  if (f->normalized == NULL)
    {
      m = strdup ("out of memory");
      goto fail;
    }
  f->api->ReleaseTypeInfo (type);
  f->api->ReleaseSessionOptions (options);
  return f;

fail:
  if (f != NULL)
    {
      if (type != NULL)
        f->api->ReleaseTypeInfo (type);
      if (options != NULL)
        f->api->ReleaseSessionOptions (options);
      release (f);
    }
  if (m == NULL)
    m = strdup ("out of memory");
  if (error != NULL)
    *error = m;
  else
    free (m);
  return NULL;
}

int
easel_finder_side (const EaselFinder *f)
{
  return f->side;
}

EaselFinderResult
easel_finder_run (EaselFinder *f, const float *in, float *out)
{
  const OrtApi *api = f->api;
  size_t n = (size_t) 3 * f->side * f->side;
  int64_t shape[4] = { 1, 3, f->side, f->side };
  OrtValue *input = NULL, *output = NULL;
  OrtStatus *status;
  const char *in_names[1] = { f->input_name }, *out_names[1] = { f->output_name };
  float *answer = NULL;
  EaselFinderResult r = EASEL_FINDER_FAILED;

  for (size_t i = 0; i < n; i++)
    f->normalized[i] = in[i] - 0.5f;
  status = api->CreateTensorWithDataAsOrtValue (f->memory, f->normalized, n * sizeof *f->normalized, shape, 4,
                                                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input);
  if (status == NULL)
    status = api->Run (f->session, f->run, in_names, (const OrtValue *const *) &input, 1, out_names, 1, &output);
  if (status != NULL)
    api->ReleaseStatus (status);            /* failed, or ended by the terminate flag: told apart below */
  else if ((status = api->GetTensorMutableData (output, (void **) &answer)) == NULL)
    {
      memcpy (out, answer, (size_t) f->side * f->side * sizeof *out);
      r = EASEL_FINDER_DONE;
    }
  else
    api->ReleaseStatus (status);
  if (output != NULL)
    api->ReleaseValue (output);
  if (input != NULL)
    api->ReleaseValue (input);
  if (r != EASEL_FINDER_DONE && __atomic_load_n (&f->cancelled, __ATOMIC_SEQ_CST))
    r = EASEL_FINDER_CANCELLED;
  return r;
}

void
easel_finder_rearm (EaselFinder *f)
{
  OrtStatus *status;

  __atomic_store_n (&f->cancelled, 0, __ATOMIC_SEQ_CST);
  status = f->api->RunOptionsUnsetTerminate (f->run);
  if (status != NULL)
    f->api->ReleaseStatus (status);
}

void
easel_finder_cancel (EaselFinder *f)
{
  OrtStatus *status;

  __atomic_store_n (&f->cancelled, 1, __ATOMIC_SEQ_CST);
  status = f->api->RunOptionsSetTerminate (f->run);

  if (status != NULL)
    f->api->ReleaseStatus (status);
}

void
easel_finder_close (EaselFinder *f)
{
  if (f != NULL)
    release (f);
}
