function varargout = qnx_rtw_info_hook(varargin)
% EXAMPLE_RTW_INFO_HOOK - Example hook file for providing Real-Time Workshop
% with the necessary target specific information regarding your target.
%
% During its build process, Real-Time Workshop checks for the existence
% of <target>_rtw_info_hook.m, where <target> is the base file name of
% the active system target file.  For example, if your system target file
% is grt.tlc, then the hook file name is grt_rtw_info_hook.m.  If the
% hook file is present (i.e., on the MATLAB path), the target specific
% information is extracted via the API found in this file.  Otherwise,
% the host computer is the assumed target.
%
% Word lengths (case 'wordlengths'):
%
% CharNumBits  : Number of bits for C 'char'  type
% ShortNumBits : Number of bits for C 'short' type
% IntNumBits   : Number of bits for C 'int'   type
% LongNumBits  : Number of bits for C 'long'  type
%
% Implementation specific properties (case 'cImplementation'):
%
% ShiftRightIntArith   : Set true if shift right on a signed integer
%                        is implemented as arithmetic shift, and false
%                        otherwise.  For example,
%
%                        int a = -8;
%                        int b;
%                        b = a >> 1;
%
%                        In the ANSI-C standard states the above example
%                        has undefined behavior.  If the result of 'b' is
%                        -4 it is safe to assume that shift right on signed
%                        integers is implemented as arithmetic shift right,
%                        and you should set the option true.
%
% Float2IntSaturates   : Conversion from float to integer automatically
%                        saturates, therefore do not generate software
%                        saturation code.
%
% IntPlusIntSaturates  : Integer addition automatically saturates,
%                        therefore do not generate software
%                        saturation code.
%
% IntTimesIntSaturates : Integer multiplication automatically saturates,
%                        therefore do not generate software
%                        saturation code.
%
% If you are not certain about the proper settings for your target,
% type 'rtwtargetsettings' in MATLAB for more details.

% Copyright 1994-2002 The MathWorks, Inc.
% $Revision: 1.1 $ $Date: 2002/03/29 19:45:58 $

Action    = varargin{1};
modelName = varargin{2};

switch Action
 case 'wordlengths'

  % specify the target word lengths
  
  value.CharNumBits  = 8;
  value.ShortNumBits = 16;
  value.IntNumBits   = 32;
  value.LongNumBits  = 32;
  varargout{1} = value;
  
 case 'cImplementation'
  
  % specify various C-language information
  
  value.ShiftRightIntArith   = true;
  value.Float2IntSaturates   = false;
  value.IntPlusIntSaturates  = false;
  value.IntTimesIntSaturates = false;
  varargout{1} = value;
  
 otherwise
  
  % Properly accommodate future releases of Real-Time Workshop
  
  varargout = [];
  
end


