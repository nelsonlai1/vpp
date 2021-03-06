/*
 * oddbuf.c - skeleton vpp engine plug-in
 *
 * Copyright (c) <current-year> <your-organization>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <oddbuf/oddbuf.h>

#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vpp/app/version.h>
#include <stdbool.h>

/* define message IDs */
#include <oddbuf/oddbuf_msg_enum.h>

/* define message structures */
#define vl_typedefs
#include <oddbuf/oddbuf_all_api_h.h>
#undef vl_typedefs

/* define generated endian-swappers */
#define vl_endianfun
#include <oddbuf/oddbuf_all_api_h.h>
#undef vl_endianfun

/* instantiate all the print functions we know about */
#define vl_print(handle, ...) vlib_cli_output (handle, __VA_ARGS__)
#define vl_printfun
#include <oddbuf/oddbuf_all_api_h.h>
#undef vl_printfun

/* Get the API version number */
#define vl_api_version(n,v) static u32 api_version=(v);
#include <oddbuf/oddbuf_all_api_h.h>
#undef vl_api_version

#define REPLY_MSG_ID_BASE omp->msg_id_base
#include <vlibapi/api_helper_macros.h>

oddbuf_main_t oddbuf_main;

/* List of message types that this plugin understands */

#define foreach_oddbuf_plugin_api_msg                           \
_(ODDBUF_ENABLE_DISABLE, oddbuf_enable_disable)

/* Action function shared between message handler and debug CLI */

int
oddbuf_enable_disable (oddbuf_main_t * omp, u32 sw_if_index,
		       int enable_disable)
{
  vnet_sw_interface_t *sw;
  int rv = 0;

  /* Utterly wrong? */
  if (pool_is_free_index (omp->vnet_main->interface_main.sw_interfaces,
			  sw_if_index))
    return VNET_API_ERROR_INVALID_SW_IF_INDEX;

  /* Not a physical port? */
  sw = vnet_get_sw_interface (omp->vnet_main, sw_if_index);
  if (sw->type != VNET_SW_INTERFACE_TYPE_HARDWARE)
    return VNET_API_ERROR_INVALID_SW_IF_INDEX;

  vnet_feature_enable_disable ("device-input", "oddbuf",
			       sw_if_index, enable_disable, 0, 0);

  return rv;
}

static clib_error_t *
oddbuf_enable_disable_command_fn (vlib_main_t * vm,
				  unformat_input_t * input,
				  vlib_cli_command_t * cmd)
{
  oddbuf_main_t *omp = &oddbuf_main;
  u32 sw_if_index = ~0;
  int enable_disable = 1;

  int rv;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "disable"))
	enable_disable = 0;
      else if (unformat (input, "%U", unformat_vnet_sw_interface,
			 omp->vnet_main, &sw_if_index))
	;
      else
	break;
    }

  if (sw_if_index == ~0)
    return clib_error_return (0, "Please specify an interface...");

  rv = oddbuf_enable_disable (omp, sw_if_index, enable_disable);

  switch (rv)
    {
    case 0:
      break;

    case VNET_API_ERROR_INVALID_SW_IF_INDEX:
      return clib_error_return
	(0, "Invalid interface, only works on physical ports");
      break;

    case VNET_API_ERROR_UNIMPLEMENTED:
      return clib_error_return (0,
				"Device driver doesn't support redirection");
      break;

    default:
      return clib_error_return (0, "oddbuf_enable_disable returned %d", rv);
    }
  return 0;
}

/* *INDENT-OFF* */
VLIB_CLI_COMMAND (oddbuf_enable_disable_command, static) =
{
  .path = "oddbuf enable-disable",
  .short_help =
  "oddbuf enable-disable <interface-name> [disable]",
  .function = oddbuf_enable_disable_command_fn,
};
/* *INDENT-ON* */

/* API message handler */
static void vl_api_oddbuf_enable_disable_t_handler
  (vl_api_oddbuf_enable_disable_t * mp)
{
  vl_api_oddbuf_enable_disable_reply_t *rmp;
  oddbuf_main_t *omp = &oddbuf_main;
  int rv;

  rv = oddbuf_enable_disable (omp, ntohl (mp->sw_if_index),
			      (int) (mp->enable_disable));

  REPLY_MACRO (VL_API_ODDBUF_ENABLE_DISABLE_REPLY);
}

/* Set up the API message handling tables */
static clib_error_t *
oddbuf_plugin_api_hookup (vlib_main_t * vm)
{
  oddbuf_main_t *omp = &oddbuf_main;
#define _(N,n)                                                  \
    vl_msg_api_set_handlers((VL_API_##N + omp->msg_id_base),     \
                           #n,					\
                           vl_api_##n##_t_handler,              \
                           vl_noop_handler,                     \
                           vl_api_##n##_t_endian,               \
                           vl_api_##n##_t_print,                \
                           sizeof(vl_api_##n##_t), 1);
  foreach_oddbuf_plugin_api_msg;
#undef _

  return 0;
}

#define vl_msg_name_crc_list
#include <oddbuf/oddbuf_all_api_h.h>
#undef vl_msg_name_crc_list

static void
setup_message_id_table (oddbuf_main_t * omp, api_main_t * am)
{
#define _(id,n,crc)   vl_msg_api_add_msg_name_crc (am, #n "_" #crc, id + omp->msg_id_base);
  foreach_vl_msg_name_crc_oddbuf;
#undef _
}

static clib_error_t *
oddbuf_init (vlib_main_t * vm)
{
  oddbuf_main_t *om = &oddbuf_main;
  clib_error_t *error = 0;
  u8 *name;

  om->vlib_main = vm;
  om->vnet_main = vnet_get_main ();

  name = format (0, "oddbuf_%08x%c", api_version, 0);

  /* Ask for a correctly-sized block of API message decode slots */
  om->msg_id_base = vl_msg_api_get_msg_ids
    ((char *) name, VL_MSG_FIRST_AVAILABLE);

  error = oddbuf_plugin_api_hookup (vm);

  /* Add our API messages to the global name_crc hash table */
  setup_message_id_table (om, &api_main);

  /* Basic setup */
  om->n_to_copy = 1;
  om->second_chunk_offset = 1;
  om->first_chunk_offset = 0;

  vec_free (name);

  return error;
}

VLIB_INIT_FUNCTION (oddbuf_init);

/* *INDENT-OFF* */
VNET_FEATURE_INIT (oddbuf, static) =
{
  .arc_name = "device-input",
  .node_name = "oddbuf",
  .runs_before = VNET_FEATURES ("ethernet-input"),
};
/* *INDENT-ON */

/* *INDENT-OFF* */
VLIB_PLUGIN_REGISTER () =
{
  .version = VPP_BUILD_VER,
  .description = "Awkward chained buffer geometry generator",
  .default_disabled = 1,
};
/* *INDENT-ON* */


static clib_error_t *
oddbuf_config_command_fn (vlib_main_t * vm,
			  unformat_input_t * input, vlib_cli_command_t * cmd)
{
  oddbuf_main_t *omp = &oddbuf_main;
  unformat_input_t _line_input, *line_input = &_line_input;

  /* Get a line of input. */
  if (!unformat_user (input, unformat_line_input, line_input))
    return 0;

  while (unformat_check_input (line_input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (line_input, "n_to_copy %d", &omp->n_to_copy))
	;
      else if (unformat (line_input, "offset %d", &omp->second_chunk_offset))
	;
      else if (unformat (line_input, "first_offset %d",
			 &omp->first_chunk_offset))
	;
      else
	break;
    }

  unformat_free (line_input);

  return 0;
}

/* *INDENT-OFF* */
VLIB_CLI_COMMAND (oddbuf_config_command, static) =
{
  .path = "oddbuf configure",
  .short_help =
  "oddbuf configure n_to_copy <nn> offset <nn> first_offset <nn>",
  .function = oddbuf_config_command_fn,
};
/* *INDENT-ON* */



/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
