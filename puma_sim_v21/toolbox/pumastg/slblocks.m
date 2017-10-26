function blkStruct = slblocks
%SLBLOCKS Defines the block library for Servotogo I/O board

% Name of the subsystem which will show up in the
% Simulink Blocksets and Toolboxes subsystem.
blkStruct.Name = sprintf('PUMA w/ STG Board');

% The function that will be called when
% the user double-clicks on this icon.
blkStruct.OpenFcn = 'pumastg_lib';

blkStruct.MaskInitialization = '';

% The argument to be set as the Mask Display for the subsystem.
% You may comment this line out if no specific mask is desired.
%blkStruct.MaskDisplay = ['plot([1:.1:5],', ...
%                         'sin(2*pi*[1:.1:5])./[1:.1:5]./[1:.1:5]);' ...
%                         'text(2.3,.5,''rtwinlib'')'];

% Define the library list for the Simulink Library browser.
% Return the name of the library model and the name for it
Browser(1).Library = 'pumastg_lib';
Browser(1).Name    = 'PUMA w/ STG Board';
Browser(1).IsFlat  = 1;% Is this library "flat" (i.e. no subsystems)?

blkStruct.Browser = Browser;
% End of slblocks
