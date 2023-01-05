classdef (StrictDefaults)SPIMasterTransfer < matlabshared.svd.SPIMasterBlock ...
        & coder.ExternalDependency
    %SPIMasterTransfer Set the logical value of a digital output pin.
    % Location (1.5.2023 on P1 laptop): C:\ProgramData\MATLAB\SupportPackages\R2020b\toolbox\target\supportpackages\raspberrypi\+codertarget\+raspi\+internal\SPIMasterTransfer.m */
    % C. Beal: Changes noted below.
    %
    % Copyright 2016-2020 The MathWorks, Inc.

    %#codegen
    
   
    properties (Nontunable)
        %SPIModule SPI module
        % CEB 1.5.2023 changed to string address instead of numerical
        SPIModule = 'SPI1';
    end
    methods
        function set.SPIModule(obj,value)
        if ~coder.target('Rtw') && ~coder.target('Sfun')
            if ~isempty(obj.Hw)
                if ~isValidSPIModule(obj.Hw,value)
                    error(message('svd:svd:ModuleNotFound','SPI',value));
                end
            end
        end
            % CEB 1.5.2023 changed to passing through the string instead of
            % converting to a numerical value
            obj.SPIModule = value;
        end
        
        % CEB 1.5.2023 updated this function to return the string instead
        % of the numerical version of the module ID
        function ret = get.SPIModule(obj)
            ret = obj.SPIModule;
        end
        
% CEB: remove old code later
%        function value = get.SPIModule(obj)
%             if ~coder.target('Rtw') && ~coder.target('Sfun')
%                 fprintf(1,'Function get.SPIModule in Rtw, obj.SPIModule is %s\n',obj.SPIModule);
%                 value = uint32(getSPIModuleNumber(obj.Hw,obj.SPIModule));
%                 fprintf(1,'Function get.SPIModule in Rtw, value is %d\n',value);
%             else
%                % For Codegen, the entire pin name needs to be passed. Eg: CS1
%                value = obj.SPIModule;
%                fprintf(1,'Function get.SPIModule in Codegen, value is %s\n',value);
%             end
%        end
    end
    
    properties (Nontunable)
        %BoardProperty Board
        BoardProperty = 'Pi 2 Model B';
        %Slave select pin
        SSPin = 'CS0';
    end
    
    properties
        Pin
    end
    
    properties (Constant, Hidden)
        % CEB 1.5.2023 added this set of strings for the module options
        SPIModuleSet = matlab.system.StringSet({'SPI0','SPI1'});
        SSPinSet = matlab.system.StringSet({'CS0','CS1','CS2'});
        BoardPropertySet = matlab.system.StringSet({'Model B Rev1','Model B Rev2', 'Model B+', 'Pi 2 Model B','Pi 3 Model B', 'Pi 3 Model B+', 'Pi Zero W','Pi 4 Model B'});
    end
    
    methods
        function set.SSPin(obj,value)
            fprintf(1,'Function set.SSPin, value is %s\n',value);
            if ~coder.target('Rtw') && ~coder.target('Sfun')
                if ~isempty(obj.Hw)
                    if ~isValidSlaveSelectPin(obj.Hw,obj.SPIModule,value)
                        error(message('svd:svd:PinNotFound',value,'SPI Master Transfer'));
                    end
                end
            end
            obj.SSPin = value;
        end
        
        function value = get.Pin(obj)
            if ~coder.target('Rtw') && ~coder.target('Sfun')
                value = uint32(getSlaveSelectPinNumber(obj.Hw,obj.SSPin));
            else
               % For Codegen, the pin name needs to be passed. Eg: CS1
               value = obj.SSPin; 
            end
        end
    end
    
    methods(Access = protected)
        function flag = isInactivePropertyImpl(obj,prop)
            % Return false if property is visible based on object 
            % configuration, for the command line and System block dialog
            flag = isInactivePropertyImpl@matlabshared.svd.SPIMasterBlock(obj, prop);
            switch prop
                % CEB 1.5.2023 removed SPIModule from the list of hidden
                % properties here
                case {'FirstBitToTransfer','Pin'}
                    flag = true;
            end
        end
    end
    
    methods
        function obj = SPIMasterTransfer(varargin)
        coder.allowpcode('plain');
        coder.cinclude('MW_SPI_Helper.h');
        obj.Hw = codertarget.raspi.internal.Hardware;
        obj.Logo = 'RASPBERRYPI';
        obj.BlockFunction = 'Transfer';
        obj.UseCustomSSPin = 'Provided by the SPI peripheral';
        setProperties(obj,nargin,varargin{:});
        end
    end
    
      methods (Access=protected)
          function maskDisplayCmds = getMaskDisplayImpl(obj)
            
            if isequal(obj.BlockFunction,'Transfer')
                BlockFunctionStr = 'Master';
            else
                BlockFunctionStr = 'Register';
            end
            BlockFunctionStr = sprintf('%s %s',BlockFunctionStr,obj.BlockFunction);
            
            maskDisplayCmds = [ ...
                ['color(''white'');', char(10)]...                                     % Fix min and max x,y co-ordinates for autoscale mask units
                ['plot([100,100,100,100],[120,120,120,120]);', char(10)]...
                ['plot([0,0,0,0],[0,0,0,0]);', char(10)]...
                ['color(''blue'');', char(10)] ...                                     % Drawing mask layout of the block
                ['text(99, 112, ''' obj.Logo ''', ''horizontalAlignment'', ''right'');', char(10)] ...
                ['color(''black'');', char(10)] ...
                ['text(50,85,''\fontsize{12}\bfSPI'',''texmode'',''on'',''horizontalAlignment'',''center'',''verticalAlignment'',''middle'');', char(10)], ...
                ['text(50,65,''\fontsize{10}\bf' BlockFunctionStr ''',''texmode'',''on'',''horizontalAlignment'',''center'',''verticalAlignment'',''middle'');', char(10)], ...
                % CEB 1.5.2023 added the following line to display the SPI module on the block icon
                ['text(50,35,''SPI: ' num2str(obj.SPIModule) ''' ,''horizontalAlignment'', ''center'');', char(10)], ...
                ['text(50,15,''Chip select: ' num2str(obj.SSPin) ''' ,''horizontalAlignment'', ''center'');', char(10)], ...
                ];
        end
    end
                
    methods(Static, Access=protected)
        function header = getHeaderImpl()
        header = matlab.system.display.Header(mfilename('class'),...
            'ShowSourceLink', false, ...
            'Title','SPI Master Transfer', ...
            'Text', ['Write data to and read data from an SPI slave device.' char(10) char(10) ...
            'The block accepts a 1-D array of data type int8, uint8, int16, uint16, int32, uint32, single or double. The block outputs a 1-D array of the same size and data type as the input values.']);
        end
        
        function [groups, PropertyListMain, SampleTimeProp] = getPropertyGroupsImpl
            
            % CEB 1.5.2023 added this module property selection in the
            % mask. It gets automatically included below as part of the
            % PropertyListMainOut, no need for a specific inclusion
            % ModuleProperty
            SPIModuleProperty = matlab.system.display.internal.Property('SPIModule', 'Description', 'SPI Module');
            SPIModulePropertyCell = {SPIModuleProperty};
            % BoardProperty
            BoardProperty = matlab.system.display.internal.Property('BoardProperty', 'Description', 'Board');
            BoardPropertyCell = {BoardProperty};
            SlavePinProperty = matlab.system.display.internal.Property('SSPin', 'Description', 'Slave select pin');
            SlavePinPropertyCell = {SlavePinProperty};
            [~, PropertyListMainOut] = matlabshared.svd.SPIBlock.getPropertyGroupsImpl;
            % CEB 1.5.2023 moved the SlavePinPropertyCell to last as it
            % made more sense in the mask property ordering
            PropertyListMainOut = [BoardPropertyCell PropertyListMainOut SlavePinPropertyCell];
            % Create mask display
            Group = matlab.system.display.Section(...
                'PropertyList',PropertyListMainOut);
            groups = Group;
            % Output property list if requested
            if nargout > 1
                PropertyListMain = PropertyListMainOut;
                SampleTimeProp = matlab.system.display.internal.Property('SampleTime', 'Description', 'Sample time');
            end
            viewPinMapAction = matlab.system.display.Action(@codertarget.raspi.blocks.openPinMap, ...
            'Alignment', 'right', ...
            'Placement','BoardProperty',...
            'Label', 'View pin map');
        matlab.system.display.internal.setCallbacks(viewPinMapAction, ...
            'SystemDeletedFcn', @codertarget.raspi.blocks.closePinMap);
        groups(1).Actions = viewPinMapAction;
        end  
    end
    
    methods (Static)
        function name = getDescriptiveName(~)
        name = 'SPI Master Transfer';
        end
        
        function b = isSupportedContext(context)
        b = context.isCodeGenTarget('rtw');
        end
        
        function updateBuildInfo(buildInfo, context)
            if context.isCodeGenTarget('rtw')
            % SPI interface
                spkgrootDir = codertarget.raspi.internal.getSpPkgRootDir;
                raspiserverDir = fullfile(raspi.internal.getRaspiRoot, 'server');
                addIncludePaths(buildInfo, raspiserverDir);
                svdDir = matlabshared.svd.internal.getRootDir;
                addIncludePaths(buildInfo, fullfile(spkgrootDir,'include'));
                buildInfo.addIncludeFiles('MW_SPI_Helper.h');
                addIncludePaths(buildInfo,fullfile(svdDir,'include'));
                addSourcePaths(buildInfo, fullfile(spkgrootDir,'src'));
                addSourceFiles(buildInfo,'MW_SPI.c', fullfile(spkgrootDir,'src'), 'BlockModules');
            end
        end
    end
end
%[EOF]
