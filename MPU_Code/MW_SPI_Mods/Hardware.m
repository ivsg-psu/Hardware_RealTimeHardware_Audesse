classdef Hardware < handle
    % Hardware Hardware base class for Raspberry Pi
    % Location (1.5.2023 on P1 laptop): C:\ProgramData\MATLAB\SupportPackages\R2020b\toolbox\realtime\targets\raspi\+codertarget\+raspi\+internal\Hardware.m */
    % C. Beal: Changes noted below.
    
    % Copyright 2015-2018 The MathWorks, Inc.
    %#codegen
    
     properties (Constant)
        %I2C
        AvailableI2CModule = [0 1];
        AvailableI2CModuleNames = {'0','1'};
        I2CModuleMaximumBusSpeedInHz = 400e3;
        I2CModuleBusSpeedInHz = 100e3;
        I2CMaxAllowedAddressBits = 7;        
        
        %SPI
        % CEB 1.5.2023 added module 1 as an available module
        AvailableSPIModule = [0 1];
        % CEB 1.5.2023 added module names to match SPIMasterTransfer block
        AvailableSPIModuleNames = {'SPI0','SPI1'};
        % CEB 1.5.2023 added SS 2 as an available pin
        AvailableSlaveSelectPins = [0 1 2];
        % CEB 1.5.2023 updated chip select pins to match SPIMasterTransfer
        % block
        AvailableSlaveSelectPinNames = {'CS0','CS1','CS2'};
        SPIModuleMaximumBusSpeedInHz = 32e6;
        SPIModuleBusSpeedInHz = 500e3;
        
        %SCI
        AvailableSCINumbers = 0;
        AvailableSCINames = {'SCI0'};
        SCIMaximumBaudrate = 230400;
    end
    
    
    methods
         
         function obj = Hardware(varargin)
            coder.allowpcode('plain');
         end
            
        % I2C interface
        function ret = getI2CModuleName(obj,I2CNumber)
            if nargin < 2
                ret = obj.AvailableI2CModuleNames;
            else
                ret = obj.AvailableI2CModuleNames{I2CNumber == obj.AvailableI2CModule};
            end
        end
         
        function ret = isValidI2CModule(obj, I2CModule)
            if isnumeric(I2CModule)
                ret = (ismember(I2CModule,obj.AvailableI2CModule)~=0);
            else
                I2CNumber = getI2CModuleNumber(obj,I2CModule);
                ret = (ismember(I2CNumber,obj.AvailableI2CModule)~=0);
            end            
        end
        
        function ret = getI2CModuleNumber(obj,I2CModuleName)
            if nargin < 2
                ret = obj.AvailableI2CModule;
            else
                for i = 1:numel(obj.AvailableI2CModuleNames)
                    if isequal(obj.AvailableI2CModuleNames{i},I2CModuleName)
                        ret = obj.AvailableI2CModule(i);
                        return;
                    end
                end
                % Not found, hence error
                error('%s not found',I2CModuleName);
            end
        end
        
        function ret = getI2CBusSpeedInHz(obj, ~)
            % Place holder to access model config set
            ret = obj.I2CModuleBusSpeedInHz;
        end
        
        function ret = getI2CMaximumBusSpeedInHz(obj, ~)
            % Place holder to access model config set
            ret = obj.I2CModuleMaximumBusSpeedInHz;
        end
        
        function ret = getI2CMaxAllowedAddressBits(obj, ~)
            % Get allowed addressing bits is allowed
            ret = obj.I2CMaxAllowedAddressBits;
        end    
       
        % SPI interface
        function ret = getSPIModuleName(obj,SPIModuleNumber)
            if nargin < 2
                ret = obj.AvailableSPIModuleNames;
            else
                ret = obj.AvailableSPIModuleNames{SPIModuleNumber == obj.AvailableSPIModule};
            end
        end
        
        function ret = isValidSPIModule(obj, SPIModule)
            if isnumeric(SPIModule)
                ret = (ismember(SPIModule,obj.AvailableSPIModule)~=0);
            else
                SPINumber = getSPIModuleNumber(obj,SPIModule);
                ret = (ismember(SPINumber,obj.AvailableSPIModule)~=0);
            end  
        end
           
        function ret = getSPIModuleNumber(obj,SPIModuleName)
            if nargin < 2
                ret = obj.AvailableSPIModule;
            else
                for i = 1:numel(obj.AvailableSPIModuleNames)
                    if isequal(obj.AvailableSPIModuleNames{i},SPIModuleName)
                        ret = obj.AvailableSPIModule(i);
                        return;
                    end
                end
                % Not found, hence error
                error('%s not found',SPIModuleName);
            end
        end
           
        function ret = getSlaveSelectPinName(obj,PinNumber)
            if nargin < 2
                ret = obj.AvailableSlaveSelectPinNames;
            else
                ret = obj.AvailableSlaveSelectPinNames{PinNumber == obj.AvailableSlaveSelectPins};
            end
        end
            
        % CEB 1.5.2023 noting that the middle argument here was originally
        % the module and could be used to avoid bad combinations of modules
        % and chip select pins. A possible upgrade for later.
        function ret = isValidSlaveSelectPin(obj,~,SPIpin)
                if isnumeric(SPIpin)
                    ret = (ismember(SPIpin,obj.AvailableSlaveSelectPins)~=0);
                else
                    SPIpinNumber = getSlaveSelectPinNumber(obj,SPIpin);
                    ret = (ismember(SPIpinNumber,obj.AvailableSlaveSelectPins)~=0);
                end  
        end
           
        function ret = getSlaveSelectPinNumber(obj, PinName)
            if nargin < 2
                ret = obj.AvailableSlaveSelectPins;
            else
                for i = 1:numel(obj.AvailableSlaveSelectPinNames)
                    if isequal(obj.AvailableSlaveSelectPinNames{i},PinName)
                        ret = obj.AvailableSlaveSelectPins(i);
                        return;
                    end
                end
                % Not found, hence error
                error('Pin %s not found',PinName);
            end
        end
        
        function ret = getSPIMaximumBusSpeedInHz(obj, ~)
            ret = obj.SPIModuleMaximumBusSpeedInHz;
        end
  
        function ret = getBusSpeedParameterVisibility(~,~)
            ret = false;
        end
        
        % Get SPI MOSI Pin
        function ret = getSPIMosiPin(~,~)
            ret = [];
        end
        % Get SPI MISO Pin
        function ret = getSPIMisoPin(~,~)
            ret = [];
        end
        % Get SPI SCK Pin
        function ret = getSPIClockPin(~,~)
            ret = [];
        end
        function ret = getSPIBusSpeedInHz(obj,~)
            ret = obj.SPIModuleBusSpeedInHz;
        end
        
         % SCI interface
        % Get SCI module name based on the identifier
        function ret = getSCIModuleName(obj,SCINumber)
            if nargin < 2
                ret = obj.AvailableSCINames(obj.AvailableSCINumbers+1);
            else
                ret = obj.AvailableSCINames{ismember(obj.AvailableSCINumbers, SCINumber)};
            end
        end
        % Validate is the SCI module available for the hardware
        function ret = isValidSCIModule(obj, value)
            validateattributes(value, {'char'}, ...
                    {'nonempty', 'row'}, '');
            ret = true;
        end
        % Get the SCI module identifier from the name
        function ret = getSCIModuleNumber(obj,SCIName)
            if nargin < 2
                ret = obj.AvailableSCINumbers;
            else
                for i = coder.unroll(1:numel(obj.AvailableSCINames))
                    if isequal(obj.AvailableSCINames{i},SCIName)
                        ret = obj.AvailableSCINumbers(i);
                        return;
                    else
                        ret = [];
                    end
                end
            end
        end
        % SCI module to consider as string
        % Linux based targets like Raspi are having virtual SCI.
        function ret = getSCIModuleNameIsString(obj)
            ret = true;
        end
        % Get the SCI recevie pin name
        function ret = getSCIReceivePin(obj,SCIModule)
            ret = [];
        end
        % Get the SCI transmit Pin name
        function ret = getSCITransmitPin(obj,SCIModule)
            ret = [];
        end
        % Get SCI bus speed
        function ret = getSCIBaudrate(obj, SCIModule)
            ret = 9600;
        end
        % Get the maximum allowed bus speed
        function ret = getSCIMaximumBaudrate(obj, SCIModule)
            ret = obj.SCIMaximumBaudrate;
        end
        % Get Data bits
        function ret = getSCIDataBits(obj, SCIModule)
            ret = 8;
        end
        % Get the parity
        function ret = getSCIParity(obj, SCIModule)
            ret = matlabshared.svd.SCI.PARITY_NONE;
        end
        % Get the stop bits
        function ret = getSCIStopBits(obj, SCIModule)
            ret = 1;
        end
        % Frame parameters visibility
        % true - visible
        % false - invisible
        function ret = getSCIParametersVisibility(obj, SCIModule)
            ret = true;
        end
        % RTS pin for hardware flow control
        function ret = getSCIRtsPin(obj, SCIModule)
            ret = [];
        end
        % CTS pin for hardware flow control
        function ret = getSCICtsPin(~, ~)
            ret = [];
        end
        % Define Hardware flow control type
        % true - Enable RTS/CTS
        % false - No flow control
        function ret = getSCIHardwareFlowControl(obj,SCIModule)
            ret = false;
        end
        % Define byte order for communicating with other SCI device
        % true - BigEndian
        % false - LittleEndian
        function ret = getSCIByteOrder(obj,SCIModule)
            ret = false;
        end
    end
end

% LocalWords:  SPI MOSI MISO SCK Raspi recevie CTS
