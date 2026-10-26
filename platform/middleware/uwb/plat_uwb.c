/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file plat_uwb.c
 * @brief UWB 中间件实现：NXP SR040（基于 nxp-uwbiot UwbApi/UCI）
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <platform/plat_uwb.h>

LOG_MODULE_REGISTER(plat_uwb);

#if DT_NODE_EXISTS(DT_NODELABEL(nxp_uwb_device))

#include "UwbApi.h"

static plat_uwb_notif_cb_t notif_cb;
static void *notif_cb_data;

/* UWB SDK 应用回调：转发给平台层注册的回调 */
static void plat_uwb_app_cb(eNotificationType op_type, void *data, size_t len)
{
	if (notif_cb != NULL) {
		notif_cb((uint16_t)op_type, data, len, notif_cb_data);
	}
}

void plat_uwb_set_notif_cb(plat_uwb_notif_cb_t cb, void *user_data)
{
	notif_cb = cb;
	notif_cb_data = user_data;
}

int plat_uwb_init(void)
{
	phUwbappContext_t app_ctx = { 0 };
	tUWBAPI_STATUS status;

	if (notif_cb != NULL) {
		app_ctx.pCallback = plat_uwb_app_cb;
	}

	status = UwbApi_Initialize(&app_ctx);
	if (status != NFCSTATUS_SUCCESS) {
		LOG_ERR("UwbApi_Initialize failed: 0x%02x", status);
		return -EIO;
	}

	LOG_INF("UWB stack initialized");
	return 0;
}

int plat_uwb_deinit(void)
{
	tUWBAPI_STATUS status;

	status = UwbApi_DeInitUwb();
	if (status != NFCSTATUS_SUCCESS) {
		LOG_WRN("UwbApi_DeInitUwb status: 0x%02x", status);
	}
	return 0;
}

int plat_uwb_get_device_info(struct plat_uwb_dev_info *info)
{
	phUwbDevInfo_t dev_info = { 0 };
	tUWBAPI_STATUS status;

	if (info == NULL) {
		return -EINVAL;
	}

	status = UwbApi_GetDeviceInfo(&dev_info);
	if (status != NFCSTATUS_SUCCESS) {
		LOG_ERR("UwbApi_GetDeviceInfo failed: 0x%02x", status);
		return -EIO;
	}

	memset(info, 0, sizeof(*info));
	info->uci_version = dev_info.uciGenericVersion;
	info->mac_major = dev_info.macMajorVersion;
	info->mac_minor = dev_info.macMinorMaintenanceVersion;
	info->phy_major = dev_info.phyMajorVersion;
	info->phy_minor = dev_info.phyMinorMaintenanceVersion;
	info->fw_major = dev_info.fwMajor;
	info->fw_minor = dev_info.fwMinor;
	info->fw_patch = dev_info.fwPatchVersion;
	info->mw_major = dev_info.mwMajor;
	info->mw_minor = dev_info.mwMinor;

	strncpy(info->dev_name, (const char *)dev_info.devName, sizeof(info->dev_name) - 1);
	info->dev_name[sizeof(info->dev_name) - 1] = '\0';
	strncpy(info->serial_no, (const char *)dev_info.serialNo, sizeof(info->serial_no) - 1);
	info->serial_no[sizeof(info->serial_no) - 1] = '\0';
	return 0;
}

int plat_uwb_session_start(uint32_t session_id, uint32_t *session_handle)
{
	tUWBAPI_STATUS status;
	uint32_t handle = 0;

	if (session_handle == NULL) {
		return -EINVAL;
	}

	status = UwbApi_SessionInit(session_id, UWBD_RANGING_SESSION, &handle);
	if (status != NFCSTATUS_SUCCESS) {
		LOG_ERR("UwbApi_SessionInit(%u) failed: 0x%02x", session_id, status);
		return -EIO;
	}

	status = UwbApi_StartRangingSession(handle);
	if (status != NFCSTATUS_SUCCESS) {
		LOG_ERR("UwbApi_StartRangingSession failed: 0x%02x", status);
		(void)UwbApi_SessionDeinit(handle);
		return -EIO;
	}

	*session_handle = handle;
	LOG_INF("UWB ranging session started, handle=0x%08x", handle);
	return 0;
}

int plat_uwb_session_stop(uint32_t session_handle)
{
	tUWBAPI_STATUS status;

	status = UwbApi_StopRangingSession(session_handle);
	if (status != NFCSTATUS_SUCCESS) {
		LOG_WRN("UwbApi_StopRangingSession failed: 0x%02x", status);
		return -EIO;
	}
	return 0;
}

int plat_uwb_session_deinit(uint32_t session_handle)
{
	tUWBAPI_STATUS status;

	status = UwbApi_SessionDeinit(session_handle);
	if (status != NFCSTATUS_SUCCESS) {
		LOG_WRN("UwbApi_SessionDeinit failed: 0x%02x", status);
		return -EIO;
	}
	return 0;
}

#else /* 未声明 nxp_uwb_device 设备树节点：桩实现 */

void plat_uwb_set_notif_cb(plat_uwb_notif_cb_t cb, void *user_data)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
}

int plat_uwb_init(void)
{
	return -ENODEV;
}

int plat_uwb_deinit(void)
{
	return -ENODEV;
}

int plat_uwb_get_device_info(struct plat_uwb_dev_info *info)
{
	ARG_UNUSED(info);
	return -ENODEV;
}

int plat_uwb_session_start(uint32_t session_id, uint32_t *session_handle)
{
	ARG_UNUSED(session_id);
	ARG_UNUSED(session_handle);
	return -ENODEV;
}

int plat_uwb_session_stop(uint32_t session_handle)
{
	ARG_UNUSED(session_handle);
	return -ENODEV;
}

int plat_uwb_session_deinit(uint32_t session_handle)
{
	ARG_UNUSED(session_handle);
	return -ENODEV;
}

#endif /* DT_NODE_EXISTS(DT_NODELABEL(nxp_uwb_device)) */
