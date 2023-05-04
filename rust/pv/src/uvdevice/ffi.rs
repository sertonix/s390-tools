// SPDX-License-Identifier: MIT
//
// Copyright IBM Corp. 2023

use crate::{assert_size, static_assert};
use zerocopy::{AsBytes, FromBytes};

pub const UVIO_ATT_ARCB_MAX_LEN: usize = 0x100000;
pub const UVIO_ATT_MEASUREMENT_MAX_LEN: usize = 0x8000;
pub const UVIO_ATT_ADDITIONAL_MAX_LEN: usize = 0x8000;
pub const UVIO_ADD_SECRET_MAX_LEN: usize = 0x100000;
pub const UVIO_LIST_SECRETS_LEN: usize = 0x1000;

// equal to ascii 'u'
pub const UVIO_TYPE_UVC: u8 = 117u8;

pub const UVIO_IOCTL_UVDEV_INFO_NR: u8 = 0;
pub const UVIO_IOCTL_ATT_NR: u8 = 1;
pub const UVIO_IOCTL_ADD_SECRET_NR: u8 = 2;
pub const UVIO_IOCTL_LIST_SECRETS_NR: u8 = 3;
pub const UVIO_IOCTL_LOCK_SECRETS_NR: u8 = 4;

/// Uvdevice IOCTL control block
/// Programs can use this struct to communicate with the uvdevice via IOCTLs
/// `argument_{addr,len}` specifies in/out data depending on the request
///
/// 'uv_rc' and `uv_rrc` are the response and reason response codes from the
/// Ultravisor.
///
/// `flags` is currently unused and to be set zero
///
#[repr(C)]
#[derive(Debug)]
pub struct uvio_ioctl_cb {
    pub flags: u32,
    pub uv_rc: u16,
    pub uv_rrc: u16,
    pub argument_addr: u64,
    pub argument_len: u32,
    pub reserved14: [u8; 44usize],
}
assert_size!(uvio_ioctl_cb, 0x40);

/// Information of supported functions by the uvdevice
///
/// * `supp_uvio_cmds` - supported IOCTLs by this device
/// * `supp_uv_cmds` - supported UVCs corresponding to the IOCTL
///
/// UVIO request to get information about supported request types by this
/// uvdevice and the Ultravisor.
/// Everything is output. Bits are in LSB0 ordering.
/// If the bit is set in both, `supp_uvio_cmds` and `supp_uv_cmds`,
/// the uvdevice and the Ultravisor support that call.
///
/// Note that bit 0 (UVIO_IOCTL_UVDEV_INFO_NR) is always zero for `supp_uv_cmds`
/// as there is no corresponding UV-call.
#[repr(C)]
#[derive(Debug, Copy, Clone, AsBytes, FromBytes)]
pub struct uvio_uvdev_info {
    pub supp_uvio_cmds: u64,
    pub supp_uv_cmds: u64,
}
assert_size!(uvio_uvdev_info, 0x10);

pub const UVIO_ATT_USER_DATA_LEN: usize = 0x100;
pub const UVIO_ATT_UID_LEN: usize = 0x10;

/// Request Attestation Measurement control block
///
/// The Attestation Request has two input and two outputs.
/// ARCB and User Data are inputs for the UV.
/// Measurement and Additional Data are outputs generated by UV.
///
/// The Attestation Request Control Block (ARCB) is a cryptographically verified
/// and secured request to UV and User Data is some plaintext data which is
/// going to be included in the Attestation Measurement calculation.
///
/// Measurement is a cryptographic measurement of the callers properties,
/// optional data configured by the ARCB and the user data. If specified by the
/// ARCB, UV will add some Additional Data to the measurement calculation.
/// This Additional Data is then returned as well.
///
/// If the Retrieve Attestation Measurement UV facility is not present,
/// UV will return invalid command rc.
/// Obviously all numbers are in BIG-endian!
#[repr(C)]
#[derive(Debug, AsBytes, FromBytes)]
pub struct uvio_attest {
    pub arcb_addr: u64,                          //in
    pub meas_addr: u64,                          //out
    pub add_data_addr: u64,                      //out
    pub user_data: [u8; UVIO_ATT_USER_DATA_LEN], //in
    pub config_uid: [u8; UVIO_ATT_UID_LEN],      //out
    pub arcb_len: u32,
    pub meas_len: u32,
    pub add_data_len: u32,
    pub user_data_len: u16,
    pub reserved136: u16,
}
assert_size!(uvio_attest, 0x138);

#[allow(dead_code)] //TODO rm when pv learns attestation
impl uvio_attest {
    pub const ARCB_MAX_LEN: usize = UVIO_ATT_ARCB_MAX_LEN;
    pub const MEASUREMENT_MAX_LEN: usize = UVIO_ATT_MEASUREMENT_MAX_LEN;
    pub const ADDITIONAL_MAX_LEN: usize = UVIO_ATT_ADDITIONAL_MAX_LEN;
}

/// corresponds to the UV_IOCTL macro
pub const fn uv_ioctl(nr: u8) -> u64 {
    iowr(UVIO_TYPE_UVC, nr, std::mem::size_of::<uvio_ioctl_cb>())
}
static_assert!(uv_ioctl(UVIO_IOCTL_ATT_NR) == 0xc0407501);

/// corresponds to the __IOWR macro
const fn iowr(ty: u8, nr: u8, size: usize) -> u64 {
    // constants and calculation from linux: asm-generic/ioctl.h
    const _IOC_WRITE: u32 = 1;
    const _IOC_READ: u32 = 2;
    const _IOC_NRSHIFT: u32 = 0;
    const _IOC_TYPESHIFT: u32 = 8;
    const _IOC_SIZESHIFT: u32 = 16;
    const _IOC_DIRSHIFT: u32 = 30;
    ((_IOC_READ | _IOC_WRITE) as u64) << _IOC_DIRSHIFT
        | ((ty as u64) << _IOC_TYPESHIFT)
        | ((nr as u64) << _IOC_NRSHIFT)
        | ((size as u64) << _IOC_SIZESHIFT)
}
