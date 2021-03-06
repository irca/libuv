/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* This benchmark spawns itself 1000 times. */

#include "task.h"
#include "uv.h"

static int N = 1000;
static int done;

static uv_process_t process;
static uv_process_options_t options = { 0 };
static char exepath[1024];
static size_t exepath_size = 1024;
static char* args[3];
static uv_pipe_t out;

#define OUTPUT_SIZE 1024
static char output[OUTPUT_SIZE];
static int output_used;

static int process_open;
static int pipe_open;


static void spawn();


void maybe_spawn() {
  if (process_open == 0 && pipe_open == 0) {
    done++;
    if (done < N) {
      spawn();
    }
  }
}


static void process_close_cb(uv_handle_t* handle) {
  ASSERT(process_open == 1);
  process_open = 0;
  maybe_spawn();
}


static void exit_cb(uv_process_t* process, int exit_status, int term_signal) {
  ASSERT(exit_status == 42);
  ASSERT(term_signal == 0);
  uv_close((uv_handle_t*)process, process_close_cb);
}


uv_buf_t on_alloc(uv_stream_t* tcp, size_t suggested_size) {
  uv_buf_t buf;
  buf.base = output + output_used;
  buf.len = OUTPUT_SIZE - output_used;
  return buf;
}


void pipe_close_cb(uv_handle_t* pipe) {
  ASSERT(pipe_open == 1);
  pipe_open = 0;
  maybe_spawn();
}


void on_read(uv_stream_t* pipe, ssize_t nread, uv_buf_t buf) {
  uv_err_t err = uv_last_error();

  if (nread > 0) {
    ASSERT(pipe_open == 1);
    output_used += nread;
  } else if (nread < 0) {
    if (err.code == UV_EOF) {
      uv_close((uv_handle_t*)pipe, pipe_close_cb);
    }
  }
}


static void spawn() {
  int r;

  ASSERT(process_open == 0);
  ASSERT(pipe_open == 0);

  args[0] = exepath;
  args[1] = "spawn_helper";
  args[2] = NULL;
  options.file = exepath;
  options.args = args;
  options.exit_cb = exit_cb;

  uv_pipe_init(&out);
  options.stdout_stream = &out;

  r = uv_spawn(&process, options);
  ASSERT(r == 0);

  process_open = 1;
  pipe_open = 1;
  output_used = 0;

  r = uv_read_start((uv_stream_t*) &out, on_alloc, on_read);
  ASSERT(r == 0);
}


BENCHMARK_IMPL(spawn) {
  int r; 
  static int64_t start_time, end_time;

  uv_init();

  r = uv_exepath(exepath, &exepath_size);
  ASSERT(r == 0);
  exepath[exepath_size] = '\0';

  uv_update_time();
  start_time = uv_now();

  spawn();

  r = uv_run();
  ASSERT(r == 0);

  uv_update_time();
  end_time = uv_now();

  LOGF("spawn: %.0f spawns/s\n",
       (double) N / (double) (end_time - start_time) * 1000.0);

  return 0;
}
