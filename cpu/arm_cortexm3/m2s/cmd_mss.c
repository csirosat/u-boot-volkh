/*
 * (C) Copyright 2020 CSIRO
 * Commonwealth Scientific and Industrial Research Organisation
 * Mike Pilawa <Mike.Pilawa@csiro.au>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/*
 *  Microsemi Smartfusion2 MSS System Services driver commands
 */
#include <common.h>
#include <command.h>
#include <stdint.h>
#include <spi.h>
#include "mss_sys_services.h"

#define	MSS_DEBUG

#ifdef	MSS_DEBUG
#define	dbg_printf(fmt,args...)	printf("%s: " fmt , __FUNCTION__, ##args)
#else
#define dbg_printf(fmt,args...)
#endif

#define argv_printf(fmt,args...) printf("%s %s: " fmt, argv[0], argv[1], ##args)

/* Local defines */
#define MSS_SUCCESS	0
#define MSS_FAILURE	1

#define SF_CMD_READ_ID	0x9f

/* Local enums */
enum mss_op_enum {
	MSS_OP_NONE,
	MSS_OP_INVALID,
	MSS_OP_GETUSR,
	MSS_OP_GETVER,
	MSS_OP_IAPAUTH,
	MSS_OP_IAPPROG,
	MSS_OP_IAPVERI,
};

/* Local string lookup tables */

#ifdef MSS_DEBUG
const char * iap_cmd_str[] = {
	[MSS_SYS_PROG_AUTHENTICATE]	= "authentication",
	[MSS_SYS_PROG_PROGRAM]		= "programming",
	[MSS_SYS_PROG_VERIFY]		= "verification",
};
#endif

const char * iap_auth_err_str[] = {
	[MSS_SYS_CHAINING_MISMATCH             ] = "CHAINING_MISMATCH",
	[MSS_SYS_UNEXPECTED_DATA_RECEIVED      ] = "UNEXPECTED_DATA_RECEIVED",
	[MSS_SYS_INVALID_ENCRYPTION_KEY        ] = "INVALID_ENCRYPTION_KEY",
	[MSS_SYS_INVALID_COMPONENT_HEADER      ] = "INVALID_COMPONENT_HEADER",
	[MSS_SYS_BACK_LEVEL_NOT_SATISFIED      ] = "BACK_LEVEL_NOT_SATISFIED",
	[MSS_SYS_DSN_BINDING_MISMATCH          ] = "DSN_BINDING_MISMATCH",
	[MSS_SYS_ILLEGAL_COMPONENT_SEQUENCE    ] = "ILLEGAL_COMPONENT_SEQUENCE",
	[MSS_SYS_INSUFFICIENT_DEV_CAPABILITIES ] = "INSUFFICIENT_DEV_CAPABILITIES",
	[MSS_SYS_INCORRECT_DEVICE_ID           ] = "INCORRECT_DEVICE_ID",
	[MSS_SYS_UNSUPPORTED_BITSTREAM_PROT_VER] = "UNSUPPORTED_BITSTREAM_PROT_VER",
	[MSS_SYS_VERIFY_NOT_PERMITTED_ON_BITSTR] = "VERIFY_NOT_PERMITTED_ON_BITSTR",
	[MSS_SYS_INVALID_DEVICE_CERTIFICATE    ] = "INVALID_DEVICE_CERTIFICATE",
};

#define MSS_IAP_AUTH_ERROR_MAX	MSS_SYS_INVALID_DEVICE_CERTIFICATE

const char * iap_prog_err_str[] = {
	[MSS_SYS_NVM_VERIFY_FAILED               - 128] = "NVM_VERIFY_FAILED",
	[MSS_SYS_DEVICE_SECURITY_PROTECTED       - 128] = "DEVICE_SECURITY_PROTECTED",
	[MSS_SYS_PROGRAMMING_MODE_NOT_ENABLED    - 128] = "PROGRAMMING_MODE_NOT_ENABLED",
	[MSS_SYS_ENVM_PROGRAMMING_OPERATION_FAIL - 128] = "ENVM_PROGRAMMING_OPERATION_FAIL",
	[MSS_SYS_ENVM_VERIFY_OPERATION_FAIL      - 128] = "ENVM_VERIFY_OPERATION_FAIL",
	[MSS_SYS_ACCESS_ERROR                    - 128] = "ACCESS_ERROR",
	[MSS_SYS_PUF_ACCESS_ERROR                - 128] = "PUF_ACCESS_ERROR",
	[MSS_SYS_BAD_COMPONENT                   - 128] = "BAD_COMPONENT",
};

#define MSS_IAP_PROG_ERROR_MAX	MSS_SYS_BAD_COMPONENT

/*
 * Control IRQs
 */
static inline void __attribute__((used)) __enable_irq(void)
{
	asm volatile ("cpsie i");
}

static inline void __attribute__((used)) __disable_irq(void)
{
	asm volatile ("cpsid i");
}

/* Local functions */

static void mss_usage (cmd_tbl_t * cmdtp)
{
	cmd_usage(cmdtp);
}

static int mss_get_usercode (cmd_tbl_t * cmdtp)
{
	int status;
	uint8_t uc[4];

	dbg_printf("will get FPGA design user-code\n");

	MSS_SYS_init(NULL);

	dbg_printf("MSS_SYS_init succeeded\n");

	__enable_irq();

	status = MSS_SYS_get_user_code(uc);

	__disable_irq();

	dbg_printf("MSS_SYS_get_user_code: status=%d, user-code=0x%08x(\"%c%c%c%c\")\n",
			status, *(unsigned int *)uc, uc[3], uc[2], uc[1], uc[0]);

	return status;
}

static int mss_get_version (cmd_tbl_t * cmdtp)
{
	int status;
	uint8_t version[2];

	dbg_printf("will get FPGA design version\n");

	MSS_SYS_init(NULL);

	dbg_printf("MSS_SYS_init succeeded\n");

	__enable_irq();

	status = MSS_SYS_get_design_version(version);

	__disable_irq();

	dbg_printf("MSS_SYS_get_design_version: status=%d, version=%hd\n",
			status, *(short *)version);

	return status;
}

static int mss_iap_cmd (cmd_tbl_t * cmdtp, int argc, char *argv[], int iap_cmd)
{
	int status;
	char *addrstr = getenv("iap_addr");
	int iap_addr;
	struct spi_slave *slave;
	uint8_t idcode[6] = {[0] = SF_CMD_READ_ID};

	if (argc > 2)
		iap_addr = (int)simple_strtoul(argv[2], NULL, 0);
	else if (addrstr)
		iap_addr = (int)simple_strtoul(addrstr, NULL, 16);
	else
		iap_addr = CONFIG_ENV_FPGA_UPDATE_OFFSET;

	dbg_printf("will start IAP %s at offset 0x%x\n", iap_cmd_str[iap_cmd], iap_addr);

	/*
	 * Do almost the same thing as spi_flash_probe() in spi_flash.c,
	 * except don't do the full probe which sets up write and erase
	 * for specific SPI flash devices, and don't free the SPI slave
	 * until done with the IAP command.
	 */

	/*
	 * Setup the SPI slave (the SPI flash) data structure.
	 * malloc used inside, so make sure to call spi_free_slave() when done.
	 */
	slave = spi_setup_slave(CONFIG_SPI_FLASH_BUS,
				CONFIG_SPI_FLASH_CS,
				CONFIG_SPI_FLASH_SPEED,
				CONFIG_SPI_FLASH_MODE);

	dbg_printf("spi_slave=0x%p\n", (void *)slave);

	if (!slave) {
		argv_printf("Failed to setup spi_slave\n");
		return MSS_FAILURE;
	}

	/*
	 * Claim the SPI bus, and set up the basic hardware modes.
	 */
	status = spi_claim_bus(slave);
	if (status) {
		argv_printf("Failed to claim SPI bus: %d\n", status);
		goto cleanup_slave;
	}

	/*
	 * Read the ID codes, mainly to finish initialization, and as a test.
	 */
	status = spi_xfer(slave, 8 * 6, idcode, idcode, SPI_XFER_BEGIN | SPI_XFER_END);
	if (status) {
		argv_printf("Failed to read flash ID: %d\n", status);
		goto cleanup_bus;
	}

	dbg_printf("Got idcode %02x %02x %02x %02x %02x\n",
			idcode[1], idcode[2], idcode[3], idcode[4], idcode[5]);

	MSS_SYS_init(NULL);

	dbg_printf("MSS_SYS_init succeeded\n");

	__enable_irq();

	status = MSS_SYS_initiate_iap(iap_cmd, iap_addr);

	__disable_irq();

	dbg_printf("MSS_SYS_initiate_iap: status=%d\n", status);

	if (status > 128) {
		if (status > MSS_IAP_PROG_ERROR_MAX)
			argv_printf("Unknown programming error! status=%d\n", status);
		else
			argv_printf("Programming error: MSS_SYS_%s\n", iap_prog_err_str[status - 128]);
	} else if (status > 0) {
		if (status > MSS_IAP_AUTH_ERROR_MAX)
			argv_printf("Unknown authentication error! status=%d\n", status);
		else
			argv_printf("Authentication error: MSS_SYS_%s\n", iap_auth_err_str[status]);
	} else if (status == 0) {
		switch (iap_cmd) {
		case MSS_SYS_PROG_AUTHENTICATE:
			argv_printf("Authentication success!\n");
			break;
		case MSS_SYS_PROG_PROGRAM:
			argv_printf("Programming success, but should not be here!!!\n");
			status = MSS_FAILURE;
			break;
		case MSS_SYS_PROG_VERIFY:
			argv_printf("Verification success!\n");
			break;
		default:
			argv_printf("Unsupported IAP command (%d)!\n", iap_cmd);
		}
	} else {
		argv_printf("Unknown problem! Negative status=%d\n", status);
	}

cleanup_bus:
	spi_release_bus(slave);

cleanup_slave:
	spi_free_slave(slave);

	return status;
}

/*
 * Decode the operation string into supported ops enums.
 */
static int mss_decode_op (char *opstr)
{
	int op;

	if (!strcmp ("getusr", opstr))
		op = MSS_OP_GETUSR;
	else if (!strcmp ("getver", opstr))
		op = MSS_OP_GETVER;
	else if (!strcmp ("iapauth", opstr))
		op = MSS_OP_IAPAUTH;
	else if (!strcmp ("iapprog", opstr))
		op = MSS_OP_IAPPROG;
	else if (!strcmp ("iapveri", opstr))
		op = MSS_OP_IAPVERI;
	else
		op = MSS_OP_INVALID;

	return op;
}

/* ------------------------------------------------------------------------- */
/* Command form:
 *   mss <op> <arg>
 * Where <op> is one of...
 *   'getusr' : Get design user-code. No <arg> required.
 *   'getver' : Get design version. No <arg> required.
 *   'iapauth': IAP authenticate FPGA image. <arg> = offset in SPI flash.
 *   'iapprog': IAP program FPGA image. <arg> = offset in SPI flash.
 *   'iapveri': IAP verify FPGA image. <arg> = offset in SPI flash.
 *              If <arg> is omitted, 'iapaddr' environment variable is used.
 */
int do_mss (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int rc = MSS_FAILURE;
	int op;

	/* Do we have enough args to parse?  If so, decode the operation. */
	op = (argc > 1) ? mss_decode_op(argv[1]) : MSS_OP_NONE;

	/* Now handle the decoded operation */
	switch (op) {
	case MSS_OP_NONE:
		printf("%s: too few args (%d)\n", argv[0], argc);
		mss_usage(cmdtp);
		break;

	case MSS_OP_GETUSR:
		rc = mss_get_usercode(cmdtp);
		break;

	case MSS_OP_GETVER:
		rc = mss_get_version(cmdtp);
		break;

	case MSS_OP_IAPAUTH:
		rc = mss_iap_cmd(cmdtp, argc, argv, MSS_SYS_PROG_AUTHENTICATE);
		break;

	case MSS_OP_IAPPROG:
		rc = mss_iap_cmd(cmdtp, argc, argv, MSS_SYS_PROG_PROGRAM);
		break;

	case MSS_OP_IAPVERI:
		rc = mss_iap_cmd(cmdtp, argc, argv, MSS_SYS_PROG_VERIFY);
		break;

	default:
		argv_printf("Invalid operation!\n");
		mss_usage(cmdtp);
		break;
	}
	return (rc);
}

U_BOOT_CMD (mss, 3, 1, do_mss,
	    "M2S MSS System Services support",
	    "[operation] <arguments>\n"
	    "  operations:\n"
	    "    getusr  :\tGet FPGA design user-code\n"
	    "    getver  :\tGet FPGA design version\n"
	    "    iapauth :\tIAP Authentication of image at SPI flash offset <arg>\n"
	    "    iapprog :\tIAP Programming of image at SPI flash offset <arg>\n"
	    "    iapveri :\tIAP Verification of image at SPI flash offset <arg>\n"
	    "             \tIf <arg> is omitted, 'iapaddr' environment variable is used.\n"
);
