# This Python file uses the following encoding: utf-8
"""autogenerated by genpy from xsens_mti_driver/XsStatusWord.msg. Do not edit."""
import codecs
import sys
python3 = True if sys.hexversion > 0x03000000 else False
import genpy
import struct


class XsStatusWord(genpy.Message):
  _md5sum = "dad684e003fb0f5d7e08711072d64f83"
  _type = "xsens_mti_driver/XsStatusWord"
  _has_header = False  # flag to mark the presence of a Header object
  _full_text = """# Define the custom XsStatusWord message
bool selftest
bool filter_valid
bool gnss_fix
uint8 no_rotation_update_status ##0: not running, 2: aborted, 3:running
bool representative_motion
bool clock_bias_estimation
#bool reserved1
bool clipflag_acc_x
bool clipflag_acc_y
bool clipflag_acc_z
bool clipflag_gyr_x
bool clipflag_gyr_y
bool clipflag_gyr_z
bool clipflag_mag_x
bool clipflag_mag_y
bool clipflag_mag_z
#uint8 reserved2
bool clipping_indication
#bool reserved3
bool syncin_marker
bool syncout_marker
uint8 filter_mode ##0: Without GNSS, 1: Coasting, 3: With GNSS
bool have_gnss_time_pulse
uint8 rtk_status ##0: No RTK, 1: RTK Floating, 2: RTK Fix
#uint8 reserved4

########################
####ref to MT Low Level Protocol Communication: https://mtidocs.xsens.com/messages
####1) Bit 0: Selftest, This flag indicates if the MT passed the self-test according to eMTS(electronic Motion Tracker Specification).
####2) Bit 1: Filter Valid, This flag indicates if input into the orientation filter is reliable and / or complete.
####3) Bit 2: GNSS fix, This flag indicates if the GNSS unit has a proper fix. 
####4) Bit 3:4: NoRotationUpdate Status, This flag indicates the status of the no rotation update procedure in the filter after the SetNoRotation message has been sent. 11: Running with no rotation assumption; 10: Rotation detected, no gyro bias estimation (sticky); 00: Estimation complete, no errors.
####5) Bit 5: Representative Motion (RepMo), Indicates if the MTi is in In-run Compass Calibration Representative Mode
####6) Bit 6: Clock Bias Estimation (ClockSync), Indicates that the Clock Bias Estimation synchronization feature is active
####7) Bit 7: Reserved, Reserved for future use
####8) Bit 8: Clipflag Acc X, If set, an out of range acceleration on the X axis is detected
####9) Bit 9: Clipflag Acc Y, If set, an out of range acceleration on the Y axis is detected
####10) Bit 10: Clipflag Acc Z, If set, an out of range acceleration on the Z axis is detected
####11) Bit 11: Clipflag Gyr X, If set, an out of range angular velocity on the X axis is detected
####12) Bit 12: Clipflag Gyr Y, If set, an out of range angular velocity on the Y axis is detected
####13) Bit 13: Clipflag Gyr Z, If set, an out of range angular velocity on the Z axis is detected
####14) Bit 14: Clipflag Mag X, If set, an out of range magnetic field on the X axis is detected
####15) Bit 15: Clipflag Mag Y, If set, an out of range magnetic field on the Y axis is detected
####16) Bit 16: Clipflag Mag Z, If set, an out of range magnetic field on the Z axis is detected
####17) Bit 17:18, Reserved, Reserved for future use
####18) Bit 19, Clipping Indication, This flag indicates going out of range of one of the sensors (it is set when one or more bits from 8:16 are set)
####19) Bit 20,  Reserved, Reserved for future use
####20) Bit 21, SyncIn Marker, When a SyncIn is detected, this bit will rise to 1. 
####21) Bit 22, SyncOut Marker, When SyncOut is active, this bit will rise to 1.
####22) Bit 23:25, Filter Mode, Indicates Filter Mode, currently only available for GNSS/INS devices:000: Without GNSS (filter profile is in VRU mode); 001: Coasting mode (GNSS has been lost <60 sec ago);011: With GNSS (default mode)
####23) Bit 26, HaveGnssTimePulse, Indicates that the 1PPS GNSS time pulse is present
####24) 27:28, RtkStatus, Indicates the availability and status of RTK: 00: No RTK; 01: RTK floating; 10: RTK fixed
####25) 29:31, Reserved, Reserved for future use
"""
  __slots__ = ['selftest','filter_valid','gnss_fix','no_rotation_update_status','representative_motion','clock_bias_estimation','clipflag_acc_x','clipflag_acc_y','clipflag_acc_z','clipflag_gyr_x','clipflag_gyr_y','clipflag_gyr_z','clipflag_mag_x','clipflag_mag_y','clipflag_mag_z','clipping_indication','syncin_marker','syncout_marker','filter_mode','have_gnss_time_pulse','rtk_status']
  _slot_types = ['bool','bool','bool','uint8','bool','bool','bool','bool','bool','bool','bool','bool','bool','bool','bool','bool','bool','bool','uint8','bool','uint8']

  def __init__(self, *args, **kwds):
    """
    Constructor. Any message fields that are implicitly/explicitly
    set to None will be assigned a default value. The recommend
    use is keyword arguments as this is more robust to future message
    changes.  You cannot mix in-order arguments and keyword arguments.

    The available fields are:
       selftest,filter_valid,gnss_fix,no_rotation_update_status,representative_motion,clock_bias_estimation,clipflag_acc_x,clipflag_acc_y,clipflag_acc_z,clipflag_gyr_x,clipflag_gyr_y,clipflag_gyr_z,clipflag_mag_x,clipflag_mag_y,clipflag_mag_z,clipping_indication,syncin_marker,syncout_marker,filter_mode,have_gnss_time_pulse,rtk_status

    :param args: complete set of field values, in .msg order
    :param kwds: use keyword arguments corresponding to message field names
    to set specific fields.
    """
    if args or kwds:
      super(XsStatusWord, self).__init__(*args, **kwds)
      # message fields cannot be None, assign default values for those that are
      if self.selftest is None:
        self.selftest = False
      if self.filter_valid is None:
        self.filter_valid = False
      if self.gnss_fix is None:
        self.gnss_fix = False
      if self.no_rotation_update_status is None:
        self.no_rotation_update_status = 0
      if self.representative_motion is None:
        self.representative_motion = False
      if self.clock_bias_estimation is None:
        self.clock_bias_estimation = False
      if self.clipflag_acc_x is None:
        self.clipflag_acc_x = False
      if self.clipflag_acc_y is None:
        self.clipflag_acc_y = False
      if self.clipflag_acc_z is None:
        self.clipflag_acc_z = False
      if self.clipflag_gyr_x is None:
        self.clipflag_gyr_x = False
      if self.clipflag_gyr_y is None:
        self.clipflag_gyr_y = False
      if self.clipflag_gyr_z is None:
        self.clipflag_gyr_z = False
      if self.clipflag_mag_x is None:
        self.clipflag_mag_x = False
      if self.clipflag_mag_y is None:
        self.clipflag_mag_y = False
      if self.clipflag_mag_z is None:
        self.clipflag_mag_z = False
      if self.clipping_indication is None:
        self.clipping_indication = False
      if self.syncin_marker is None:
        self.syncin_marker = False
      if self.syncout_marker is None:
        self.syncout_marker = False
      if self.filter_mode is None:
        self.filter_mode = 0
      if self.have_gnss_time_pulse is None:
        self.have_gnss_time_pulse = False
      if self.rtk_status is None:
        self.rtk_status = 0
    else:
      self.selftest = False
      self.filter_valid = False
      self.gnss_fix = False
      self.no_rotation_update_status = 0
      self.representative_motion = False
      self.clock_bias_estimation = False
      self.clipflag_acc_x = False
      self.clipflag_acc_y = False
      self.clipflag_acc_z = False
      self.clipflag_gyr_x = False
      self.clipflag_gyr_y = False
      self.clipflag_gyr_z = False
      self.clipflag_mag_x = False
      self.clipflag_mag_y = False
      self.clipflag_mag_z = False
      self.clipping_indication = False
      self.syncin_marker = False
      self.syncout_marker = False
      self.filter_mode = 0
      self.have_gnss_time_pulse = False
      self.rtk_status = 0

  def _get_types(self):
    """
    internal API method
    """
    return self._slot_types

  def serialize(self, buff):
    """
    serialize message into buffer
    :param buff: buffer, ``StringIO``
    """
    try:
      _x = self
      buff.write(_get_struct_21B().pack(_x.selftest, _x.filter_valid, _x.gnss_fix, _x.no_rotation_update_status, _x.representative_motion, _x.clock_bias_estimation, _x.clipflag_acc_x, _x.clipflag_acc_y, _x.clipflag_acc_z, _x.clipflag_gyr_x, _x.clipflag_gyr_y, _x.clipflag_gyr_z, _x.clipflag_mag_x, _x.clipflag_mag_y, _x.clipflag_mag_z, _x.clipping_indication, _x.syncin_marker, _x.syncout_marker, _x.filter_mode, _x.have_gnss_time_pulse, _x.rtk_status))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(locals().get('_x', self)))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(locals().get('_x', self)))))

  def deserialize(self, str):
    """
    unpack serialized message in str into this message instance
    :param str: byte array of serialized message, ``str``
    """
    if python3:
      codecs.lookup_error("rosmsg").msg_type = self._type
    try:
      end = 0
      _x = self
      start = end
      end += 21
      (_x.selftest, _x.filter_valid, _x.gnss_fix, _x.no_rotation_update_status, _x.representative_motion, _x.clock_bias_estimation, _x.clipflag_acc_x, _x.clipflag_acc_y, _x.clipflag_acc_z, _x.clipflag_gyr_x, _x.clipflag_gyr_y, _x.clipflag_gyr_z, _x.clipflag_mag_x, _x.clipflag_mag_y, _x.clipflag_mag_z, _x.clipping_indication, _x.syncin_marker, _x.syncout_marker, _x.filter_mode, _x.have_gnss_time_pulse, _x.rtk_status,) = _get_struct_21B().unpack(str[start:end])
      self.selftest = bool(self.selftest)
      self.filter_valid = bool(self.filter_valid)
      self.gnss_fix = bool(self.gnss_fix)
      self.representative_motion = bool(self.representative_motion)
      self.clock_bias_estimation = bool(self.clock_bias_estimation)
      self.clipflag_acc_x = bool(self.clipflag_acc_x)
      self.clipflag_acc_y = bool(self.clipflag_acc_y)
      self.clipflag_acc_z = bool(self.clipflag_acc_z)
      self.clipflag_gyr_x = bool(self.clipflag_gyr_x)
      self.clipflag_gyr_y = bool(self.clipflag_gyr_y)
      self.clipflag_gyr_z = bool(self.clipflag_gyr_z)
      self.clipflag_mag_x = bool(self.clipflag_mag_x)
      self.clipflag_mag_y = bool(self.clipflag_mag_y)
      self.clipflag_mag_z = bool(self.clipflag_mag_z)
      self.clipping_indication = bool(self.clipping_indication)
      self.syncin_marker = bool(self.syncin_marker)
      self.syncout_marker = bool(self.syncout_marker)
      self.have_gnss_time_pulse = bool(self.have_gnss_time_pulse)
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e)  # most likely buffer underfill


  def serialize_numpy(self, buff, numpy):
    """
    serialize message with numpy array types into buffer
    :param buff: buffer, ``StringIO``
    :param numpy: numpy python module
    """
    try:
      _x = self
      buff.write(_get_struct_21B().pack(_x.selftest, _x.filter_valid, _x.gnss_fix, _x.no_rotation_update_status, _x.representative_motion, _x.clock_bias_estimation, _x.clipflag_acc_x, _x.clipflag_acc_y, _x.clipflag_acc_z, _x.clipflag_gyr_x, _x.clipflag_gyr_y, _x.clipflag_gyr_z, _x.clipflag_mag_x, _x.clipflag_mag_y, _x.clipflag_mag_z, _x.clipping_indication, _x.syncin_marker, _x.syncout_marker, _x.filter_mode, _x.have_gnss_time_pulse, _x.rtk_status))
    except struct.error as se: self._check_types(struct.error("%s: '%s' when writing '%s'" % (type(se), str(se), str(locals().get('_x', self)))))
    except TypeError as te: self._check_types(ValueError("%s: '%s' when writing '%s'" % (type(te), str(te), str(locals().get('_x', self)))))

  def deserialize_numpy(self, str, numpy):
    """
    unpack serialized message in str into this message instance using numpy for array types
    :param str: byte array of serialized message, ``str``
    :param numpy: numpy python module
    """
    if python3:
      codecs.lookup_error("rosmsg").msg_type = self._type
    try:
      end = 0
      _x = self
      start = end
      end += 21
      (_x.selftest, _x.filter_valid, _x.gnss_fix, _x.no_rotation_update_status, _x.representative_motion, _x.clock_bias_estimation, _x.clipflag_acc_x, _x.clipflag_acc_y, _x.clipflag_acc_z, _x.clipflag_gyr_x, _x.clipflag_gyr_y, _x.clipflag_gyr_z, _x.clipflag_mag_x, _x.clipflag_mag_y, _x.clipflag_mag_z, _x.clipping_indication, _x.syncin_marker, _x.syncout_marker, _x.filter_mode, _x.have_gnss_time_pulse, _x.rtk_status,) = _get_struct_21B().unpack(str[start:end])
      self.selftest = bool(self.selftest)
      self.filter_valid = bool(self.filter_valid)
      self.gnss_fix = bool(self.gnss_fix)
      self.representative_motion = bool(self.representative_motion)
      self.clock_bias_estimation = bool(self.clock_bias_estimation)
      self.clipflag_acc_x = bool(self.clipflag_acc_x)
      self.clipflag_acc_y = bool(self.clipflag_acc_y)
      self.clipflag_acc_z = bool(self.clipflag_acc_z)
      self.clipflag_gyr_x = bool(self.clipflag_gyr_x)
      self.clipflag_gyr_y = bool(self.clipflag_gyr_y)
      self.clipflag_gyr_z = bool(self.clipflag_gyr_z)
      self.clipflag_mag_x = bool(self.clipflag_mag_x)
      self.clipflag_mag_y = bool(self.clipflag_mag_y)
      self.clipflag_mag_z = bool(self.clipflag_mag_z)
      self.clipping_indication = bool(self.clipping_indication)
      self.syncin_marker = bool(self.syncin_marker)
      self.syncout_marker = bool(self.syncout_marker)
      self.have_gnss_time_pulse = bool(self.have_gnss_time_pulse)
      return self
    except struct.error as e:
      raise genpy.DeserializationError(e)  # most likely buffer underfill

_struct_I = genpy.struct_I
def _get_struct_I():
    global _struct_I
    return _struct_I
_struct_21B = None
def _get_struct_21B():
    global _struct_21B
    if _struct_21B is None:
        _struct_21B = struct.Struct("<21B")
    return _struct_21B
