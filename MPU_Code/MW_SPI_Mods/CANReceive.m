classdef CANReceive < codertarget.raspi.internal.SPIMasterTransfer ...
        & matlabshared.svd.BlockSampleTime
    %
    % CAN Message receive block for Raspberry Pi.
    %
    
    % Copyright 2019 The MathWorks, Inc.
    %#codegen
    %#ok<*EMCA>n
    
    properties
        %Identifier Type
        identifierType = 'Standard (11-bit identifier)';
    end
        
    properties(Nontunable)
        %Output data type
        outputDataType = 'Raw data';
        %Message ID
        messageId = 100;
        %Length (bytes)
        msgLength = 8;
        %Slave select pin is available at codertarget.raspi.internal.SPIMasterTransfer
    end
    
    properties (Logical, Nontunable)
        %Output Error
        outputError = false;
        %Output Remote
        outputRemote = false;
    end
    
    properties (Constant = true,Hidden)
        REG_BFPCTRL     =   hex2dec('0C')
        REG_TXRTSCTRL   =   hex2dec('0D')
        REG_CANCTRL     =   hex2dec('0F')
        REG_CNF3        =   hex2dec('28')
        REG_CNF2        =   hex2dec('29')
        REG_CNF1        =   hex2dec('2A')
        REG_CANINTE     =   hex2dec('2B')
        FLAG_EXIDE      =   hex2dec('08')
        FLAG_RXM0       =   hex2dec('20')
        FLAG_RXM1       =   hex2dec('40')
        FLAG_ERRIE      =   hex2dec('20')
    end
    
    
    properties(Hidden, Transient, Constant)
        identifierTypeSet = matlab.system.StringSet({'Standard (11-bit identifier)', 'Extended (29-bit identifier)'});
        outputDataTypeSet = matlab.system.StringSet({'Raw data', 'CAN Msg'});
    end
    
    properties(Hidden, Access = protected)
        previousData;
    end
    
    properties(Hidden, Transient)
        setidentifierType = false;
    end
    
    methods(Static)
        function a =FLAG_RXnIE(n)
            a=bitshift(01 , n);
        end
        function a =REG_RXFnSIDH(n)
            if n < 3
                a=(00+ (n * 4));
            else
                a=(4+ (n * 4));
            end
        end
        function a =REG_RXFnSIDL(n)
            if n < 3
                a=(hex2dec('01') + (n * 4));
            else
                a=(4 + hex2dec('01') + (n * 4));
            end
        end
        function a =REG_RXFnEID8(n)
            if n<3
                a=(02 + (n * 4));
            else
                a=(4+02 + (n * 4));
            end
        end
        function a =REG_RXFnEID0(n)
            if n<3
                a=(hex2dec('03') + (n * 4));
            else
                a=(4+hex2dec('03') + (n * 4));
            end
        end
        function a =REG_RXMnSIDH(n)
            a=(hex2dec('20') + (n * hex2dec('04')));
        end
        function a =REG_RXMnSIDL(n)
            a=(hex2dec('21') + (n * hex2dec('04')));
        end
        function a =REG_RXMnEID8(n)
            a=(hex2dec('22') + (n * hex2dec('04')));
        end
        function a =REG_RXMnEID0(n)
            a=(hex2dec('23') + (n * hex2dec('04')));
        end
        function a =REG_RXBnCTRL(n)
            a=(hex2dec('60') + (n * hex2dec('10')));
        end
        
        function a =REG_RXBnSIDL(n)
            a=(hex2dec('62') + (n * hex2dec('10')));
        end
    end
    
    methods
        function obj = CANReceive(varargin)
            coder.allowpcode('plain');
            setProperties(obj,nargin,varargin{:});
        end
        
        function set.identifierType(obj,value)
            obj.identifierType = value;
            obj.setidentifierType = true; %#ok<MCSUP>
        end
        
        function set.messageId(obj,value)
            if strcmp(obj.identifierType, 'Standard (11-bit identifier)') %#ok<MCSUP>
                msgIdLimit = hex2dec('7FF');
            else
                msgIdLimit = hex2dec('1FFFFFFF');
            end
            if obj.setidentifierType %#ok<MCSUP>
                validateattributes(value,...
                    {'numeric'},...
                    {'real','nonnegative','integer','scalar','<=',msgIdLimit},...
                    '', ...
                    'Message ID');
            end
            
            obj.messageId = value;
        end
        
        function set.msgLength(obj,value)
            validateattributes(value,...
                {'numeric'},...
                {'real','nonnegative','integer','scalar','<=',8},...
                '', ...
                'Message Length');
            obj.msgLength = value;
        end
    end
    
    methods(Access = protected)
        function setupImpl(obj)
            %This will open the SPI bus
            setupImpl@matlabshared.svd.SPIBlock(obj);
            
            %Initialization sequence for MCP2515
            coder.cinclude("MW_MCP2515_CAN.h");
            if coder.target('Rtw')
                isMCPInitialized = uint8(0);
                isMCPRxInitialized = uint8(0);
                
                %Initialization sequence for MCP2515
                isMCPInitialized = coder.ceval('getMCPInitStatus');
                if (isMCPInitialized == 0)
                    MCP_CANInit(obj);
                    coder.ceval('setMCPInitStatus');
                end
                
                %Initialization sequence for CANReceive block
                isMCPRxInitialized = coder.ceval('getMCPRxInitStatus');
                if (isMCPRxInitialized == 0)
                    %Set receive filters
                    MCP_CANReceiveInit(obj);
                    coder.ceval('MW_CANInitializeInterrupt');
                    coder.ceval('setMCPRxInitStatus');
                end
                
                %Save the required ID and message length in global array
                if strcmp(obj.outputDataType,'Raw data')
                    extendeFlag = uint8(0);
                    if strcmp(obj.identifierType,'Extended (29-bit identifier)')
                        extendeFlag = uint8(1);
                    end
                    coder.ceval('MW_CANAssignIdAndLength',uint32(obj.messageId),uint8(extendeFlag),uint8(obj.msgLength));
                end
            end
            obj.previousData = zeros(1,8,'uint8');
        end
        
        function  varargout = stepImpl(obj)
            idx = 3;
            if coder.target('Rtw')
                rx = parsePacket(obj); 
                
                if strcmp(obj.outputDataType,'Raw data')
                    %Output as Raw data
                    varargout{1}     = uint8(rx.data(1:obj.msgLength));
                    varargout{2}     = uint8(rx.status);
                    
                    if obj.outputError
                        varargout{idx} = uint8(rx.error);
                        idx            = idx + 1;
                    end
                    
                    if obj.outputRemote
                        remote         = rx.rtr;
                        varargout{idx} = uint8(remote);
                    end
                    obj.previousData = rx.data;
                else
                    %Output as CAN Msg
                    varargout{1} = struct('Extended', uint8(0),...
                        'Length',   uint8(0),...
                        'Remote',   uint8(0),...
                        'Error',    uint8(0),...
                        'ID',       uint32(0),...
                        'Timestamp',0,...
                        'Data',     uint8(zeros(1,obj.msgLength)')...
                        );
                    
                    varargout{1}.Extended  = uint8(rx.extended);
                    varargout{1}.Length    = rx.dataLength;
                    varargout{1}.Remote    = uint8(rx.rtr);
                    varargout{1}.Error     = rx.error;
                    varargout{1}.ID        = uint32(rx.id);
                    varargout{1}.Timestamp = 0;
                    varargout{1}.Data      = uint8(rx.data(1:obj.msgLength)');
                    varargout{2}           = uint8(rx.status);
                end
            else
                if strcmp(obj.outputDataType,'Raw data')
                    varargout{1} = uint8(zeros(1,obj.msgLength));
                    varargout{2} = uint8(0);
                    
                    if obj.outputError
                        varargout{idx} = false;
                        idx            = idx + 1;
                    end
                    if obj.outputRemote
                        remote         = false;
                        varargout{idx} = logical(remote);
                    end
                else
                    varargout{1}.Extended  = uint8(0);
                    varargout{1}.Length    = uint8(0);
                    varargout{1}.Remote    = uint8(0);
                    varargout{1}.Error     = uint8(0);
                    varargout{1}.ID        = uint32(0);
                    varargout{1}.Timestamp = 0;
                    varargout{1}.Data      = uint8(zeros(1,obj.msgLength)');
                    varargout{2}           = 0;
                end
            end
            
        end
        
        function releaseImpl(obj) %#ok<MANU>
            %TODO: check if mcp reset is required
        end
        
        function MCP_CANInit(obj)
            MODE_RESET = hex2dec('C0');
            MODE_MASK = hex2dec('E0');
            MODE_CONFIG = hex2dec('80');
            MODE_NORMAL = hex2dec('00');
            
            %Reset MCP
            writeRead(obj,uint8(MODE_RESET),'uint8');
            
            %Change to configuration mode
            modifyCmd = uint8([5, obj.REG_CANCTRL, MODE_MASK, MODE_CONFIG]);
            writeRead(obj,uint8(modifyCmd),'uint8');
            
            %Set CAN baud rate and crystal settings from a lookup table.
            cnfValues = uint8([0 0 0]);
            coder.ceval('MW_GetCANBaud',coder.wref(cnfValues(1)),coder.wref(cnfValues(2)),coder.wref(cnfValues(3)));
            if ((cnfValues(1) == 0) && (cnfValues(2) == 0) && (cnfValues(3) == 0))
                %TODO Error here
                fprintf('Unidentified baud rate value \n');
            else
                rawSpiCmd = uint8([2, obj.REG_CNF1, cnfValues(1)]);
                writeRead(obj,uint8(rawSpiCmd),'uint8');
                rawSpiCmd = uint8([2, obj.REG_CNF2, cnfValues(2)]);
                writeRead(obj,uint8(rawSpiCmd),'uint8');
                rawSpiCmd = uint8([2, obj.REG_CNF3, cnfValues(3)]);
                writeRead(obj,uint8(rawSpiCmd),'uint8');
            end
            
            %Change the CAN mode back to normal
            modifyCmd = uint8([5, obj.REG_CANCTRL , MODE_MASK, MODE_NORMAL]);
            writeRead(obj,uint8(modifyCmd),'uint8');
        end
        
        function MCP_CANReceiveInit(obj)
            %Iniit function specific for CAN Receive block
            MODE_MASK   = hex2dec('E0');
            MODE_CONFIG = hex2dec('80');
            MODE_NORMAL = hex2dec('00');
            
            allowAll        = uint8(0);
            buffer0Extended = uint8(0);
            mask0           = uint32(0);
            filter0         = uint32(0);
            filter1         = uint32(0);
            buffer1Extended = uint8(0);
            mask1           = uint32(0);
            filter2         = uint32(0);
            filter3         = uint32(0);
            filter4         = uint32(0);
            filter5         = uint32(0);
            
            coder.ceval('MW_GetCANFilters',coder.wref(allowAll),coder.wref(buffer0Extended),coder.wref(mask0),coder.wref(filter0), ...
                coder.wref(filter1), coder.wref(buffer1Extended), coder.wref(mask1), coder.wref(filter2), coder.wref(filter3), ...
                coder.wref(filter4), coder.wref(filter5));
            
            %Change to configuration mode
            rawSpiCmd = uint8([5, obj.REG_CANCTRL, MODE_MASK, MODE_CONFIG]);
            writeRead(obj,uint8(rawSpiCmd),'uint8');
            
            %Enable receive buffer full interrupt  and interrupt on EFLG error condition change
            regValue  = uint8(bitor(obj.FLAG_ERRIE, bitor(obj.FLAG_RXnIE(1), obj.FLAG_RXnIE(0))));
            rawSpiCmd = uint8([2, obj.REG_CANINTE, regValue]);
            writeRead(obj,uint8(rawSpiCmd),'uint8');
            
            %Pin settings
            rawSpiCmd = uint8([2, obj.REG_BFPCTRL, hex2dec('00')]);
            writeRead(obj,uint8(rawSpiCmd),'uint8');
            rawSpiCmd = uint8([2, obj.REG_TXRTSCTRL, hex2dec('00')]);
            writeRead(obj,uint8(rawSpiCmd),'uint8');
            
            %Apply filter settings
            if allowAll
                rawSpiCmd = uint8([2, obj.REG_RXBnCTRL(0), bitor(obj.FLAG_RXM1, obj.FLAG_RXM0)]);
                writeRead(obj,uint8(rawSpiCmd),'uint8');
                
                rawSpiCmd = uint8([2, obj.REG_RXBnCTRL(1), bitor(obj.FLAG_RXM1, obj.FLAG_RXM0)]);
                writeRead(obj,uint8(rawSpiCmd),'uint8');
            else
                if buffer0Extended
                    setExtendedFilter0(obj,mask0,filter0,filter1);
                else
                    setNormalFilter0(obj,mask0,filter0,filter1);
                end
                
                if buffer1Extended
                    setExtendedFilter1(obj,mask1,filter2,filter3,filter4,filter5);
                else
                    setNormalFilter1(obj,mask1,filter2,filter3,filter4,filter5);
                end
            end
            
            %Change the CAN mode back to normal
            modifyCmd = uint8([5, obj.REG_CANCTRL , MODE_MASK, MODE_NORMAL]);
            writeRead(obj,uint8(modifyCmd),'uint8');
        end
        
        function modifyMCPRegister(obj,register,mask,value)
            dataRaw = uint8([5 register mask value]);
            writeRead(obj,dataRaw, 'uint8');
        end
        
        function receivedFrame = parsePacket(obj)
            rxid       = uint32(0);
            error      = uint8(0);
            rxData     = zeros(1,8,'uint8');
            status     = uint8(0);
            remote     = uint8(0);
            extended   = uint8(0);
            dataLength = uint8(0);
            %fprintf(1,'Function parsePacket starting\r\n');
            
            coder.cinclude("MW_MCP2515_CAN.h");
            if strcmp(obj.outputDataType,'Raw data')
                if strcmp(obj.identifierType, 'Extended (29-bit identifier)')
                    extended = uint8(1);
                end
                rxid       = uint32(obj.messageId);
                dataLength = uint8(obj.msgLength);
                %coder.ceval('MW_GetCANMessageWithID',uint32(obj.messageId),coder.wref(rxData(1)),dataLength,coder.wref(status),extended,coder.wref(remote),coder.wref(error));
                coder.ceval('MW_PollForCANMessage',uint32(obj.messageId),coder.wref(rxData(1)),dataLength,coder.wref(status),extended,coder.wref(remote),coder.wref(error));
            else
                coder.ceval('MW_GetCANMessageNew',coder.wref(rxid),coder.wref(rxData(1)),coder.wref(dataLength),coder.wref(status),coder.wref(extended),coder.wref(remote),coder.wref(error));
            end
            
            receivedFrame.id         = rxid;
            receivedFrame.rtr        = remote;
            receivedFrame.data       = rxData;
            receivedFrame.error      = error;
            receivedFrame.status     = status;
            receivedFrame.extended   = extended;
            receivedFrame.dataLength = dataLength;
            %fprintf(1,'Function parsePacket ending with rcvd id %x\r\n',receivedFrame.id);
            %Error
            %   bit 0 RXWAR:    Receive Error Warning Flag bit
            %   bit 1 RXEP:     Receive Error-Passive Flag bit
            %   bit 2 RX0OVR:   Receive Buffer 0 Overflow Flag bit
            %   bit 3 RX1OVR:   Receive Buffer 1 Overflow Flag bit
        end
        
        function setFilter(obj,mask0,filter0,filter1,mask1,filter2,filter3,filter4,filter5)
            
            mask0 = bitand(mask0 , hex2dec('7ff'));
            mask1 = bitand(mask1 , hex2dec('7ff'));
            filter0 = bitand(filter0 , hex2dec('7ff'));
            filter1 = bitand(filter1 , hex2dec('7ff'));
            filter2 = bitand(filter2 , hex2dec('7ff'));
            filter3 = bitand(filter3 , hex2dec('7ff'));
            filter4 = bitand(filter4 , hex2dec('7ff'));
            filter5 = bitand(filter5 , hex2dec('7ff'));
            
            writeRegister(obj,obj.REG_RXBnCTRL(0), obj.FLAG_RXM0, 'uint8');
            writeRegister(obj,obj.REG_RXBnCTRL(1), obj.FLAG_RXM0, 'uint8');
            
            writeRegister(obj,obj.REG_RXMnSIDH(0), floor(bitshift(mask0,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXMnSIDL(0), bitand(bitshift(mask0,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID8(0), 0, 'uint8');
            writeRegister(obj,obj.REG_RXMnEID0(0), 0, 'uint8');
            
            writeRegister(obj,obj.REG_RXMnSIDH(1), floor(bitshift(mask1,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXMnSIDL(1), bitand(bitshift(mask1,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID8(1), 0, 'uint8');
            writeRegister(obj,obj.REG_RXMnEID0(1), 0, 'uint8');
            
            
            writeRegister(obj,obj.REG_RXFnSIDH(0), floor(bitshift(filter0,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(0), bitand(bitshift(filter0,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(0), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(0), 0, 'uint8');
            
            writeRegister(obj,obj.REG_RXFnSIDH(1), floor(bitshift(filter1,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(1), bitand(bitshift(filter1,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(1), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(1), 0, 'uint8');
            
            writeRegister(obj,obj.REG_RXFnSIDH(2), floor(bitshift(filter2,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(2), bitand(bitshift(filter2,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(2), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(2), 0, 'uint8');
            
            writeRegister(obj,obj.REG_RXFnSIDH(3), floor(bitshift(filter3,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(3), bitand(bitshift(filter3,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(3), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(3), 0, 'uint8');
            
            writeRegister(obj,obj.REG_RXFnSIDH(4), floor(bitshift(filter4,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(4), bitand(bitshift(filter4,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(4), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(4), 0, 'uint8');
            
            writeRegister(obj,obj.REG_RXFnSIDH(5), floor(bitshift(filter5,-3)), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(5), bitand(bitshift(filter5,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(5), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(5), 0, 'uint8');
        end
        
        
        
        function setNormalFilter0(obj,mask0,filter0,filter1)
            mask0 = bitand(mask0 , hex2dec('7ff'));
            filter0 = bitand(filter0 , hex2dec('7ff'));
            filter1 = bitand(filter1 , hex2dec('7ff'));
            
            % Receive only valid messages with standard identifiers that meet filter criteria
            writeRegister(obj,obj.REG_RXBnCTRL(0), obj.FLAG_RXM0, 'uint8');
            
            % Mask0 Configuration
            writeRegister(obj,obj.REG_RXMnSIDH(0), bitand(bitshift(mask0,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnSIDL(0), bitand(bitshift(mask0,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID8(0), 0, 'uint8');
            writeRegister(obj,obj.REG_RXMnEID0(0), 0, 'uint8');
            
            % Filter0 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(0), bitand(bitshift(filter0,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(0), bitand(bitshift(filter0,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(0), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(0), 0, 'uint8');
            
            % Filter1 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(1), bitand(bitshift(filter1,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(1), bitand(bitshift(filter1,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(1), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(1), 0, 'uint8');
        end
        
        function setExtendedFilter0(obj,mask0,filter0,filter1)
            mask0   = bitand(mask0 , hex2dec('1FFFFFFF'));
            filter0 = bitand(filter0 , hex2dec('1FFFFFFF'));
            filter1 = bitand(filter1 , hex2dec('1FFFFFFF'));
            
            % Receive only valid messages with extended identifiers that meet filter criteria
            writeRegister(obj, obj.REG_RXBnCTRL(0), obj.FLAG_RXM1, 'uint8');
            
            % Mask0 Configuration
            writeRegister(obj,obj.REG_RXMnSIDH(0), bitand(bitshift(mask0,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnSIDL(0), bitand(bitor(bitor(bitshift(bitand(bitshift(mask0,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(mask0,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID8(0), bitand(bitshift(mask0,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID0(0), bitand(mask0,255), 'uint8');
            
            % Filter0 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(0), bitand(bitshift(filter0,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(0), bitand(bitor(bitor(bitshift(bitand(bitshift(filter0,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(filter0,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(0), bitand(bitshift(filter0,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(0), bitand(filter0,255), 'uint8');
            
            % Filter1 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(1), bitand(bitshift(filter1,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(1), bitand(bitor(bitor(bitshift(bitand(bitshift(filter1,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(filter1,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(1), bitand(bitshift(filter1,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(1), bitand(filter1,255), 'uint8');
        end
        
        function setNormalFilter1(obj,mask1,filter2,filter3,filter4,filter5)
            mask1   = bitand(mask1 , hex2dec('7ff'));
            filter2 = bitand(filter2 , hex2dec('7ff'));
            filter3 = bitand(filter3 , hex2dec('7ff'));
            filter4 = bitand(filter4 , hex2dec('7ff'));
            filter5 = bitand(filter5 , hex2dec('7ff'));
            
            % Receive only valid messages with standard identifiers that meet filter criteria
            writeRegister(obj,obj.REG_RXBnCTRL(1), obj.FLAG_RXM0, 'uint8');
            
            % Mask1 Configuration
            writeRegister(obj,obj.REG_RXMnSIDH(1), bitand(bitshift(mask1,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnSIDL(1), bitand(bitshift(mask1,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID8(1), 0, 'uint8');
            writeRegister(obj,obj.REG_RXMnEID0(1), 0, 'uint8');
            
            % Filter2 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(2), bitand(bitshift(filter2,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(2), bitand(bitshift(filter2,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(2), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(2), 0, 'uint8');
            
            % Filter3 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(3), bitand(bitshift(filter3,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(3), bitand(bitshift(filter3,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(3), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(3), 0, 'uint8');
            
            % Filter4 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(4), bitand(bitshift(filter4,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(4), bitand(bitshift(filter4,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(4), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(4), 0, 'uint8');
            
            % Filter5 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(5), bitand(bitshift(filter5,-3),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(5), bitand(bitshift(filter5,5),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(5), 0, 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(5), 0, 'uint8');
        end
        
        function setExtendedFilter1(obj,mask1,filter2,filter3,filter4,filter5)
            mask1   = bitand(mask1 , hex2dec('1FFFFFFF'));
            filter2 = bitand(filter2 , hex2dec('1FFFFFFF'));
            filter3 = bitand(filter3 , hex2dec('1FFFFFFF'));
            filter4 = bitand(filter4 , hex2dec('1FFFFFFF'));
            filter5 = bitand(filter5 , hex2dec('1FFFFFFF'));
            
            % Receive only valid messages with extended identifiers that meet filter criteria
            writeRegister(obj,obj.REG_RXBnCTRL(1), obj.FLAG_RXM1, 'uint8');
            
            % Mask1 Configuration
            writeRegister(obj,obj.REG_RXMnSIDH(1), bitand(bitshift(mask1,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnSIDL(1), bitand(bitor(bitor(bitshift(bitand(bitshift(mask1,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(mask1,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID8(1), bitand(bitshift(mask1,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXMnEID0(1), bitand(mask1,255), 'uint8');
            
            % Filter2 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(2), bitand(bitshift(filter2,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(2), bitand(bitor(bitor(bitshift(bitand(bitshift(filter2,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(filter2,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(2), bitand(bitshift(filter2,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(2), bitand(filter2,255), 'uint8');
            
            % Filter3 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(3), bitand(bitshift(filter3,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(3), bitand(bitor(bitor(bitshift(bitand(bitshift(filter3,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(filter3,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(3), bitand(bitshift(filter3,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(3), bitand(filter3,255), 'uint8');
            
            % Filter4 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(4), bitand(bitshift(filter4,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(4), bitand(bitor(bitor(bitshift(bitand(bitshift(filter4,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(filter4,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(4), bitand(bitshift(filter4,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(4), bitand(filter4,255), 'uint8');
            
            % Filter5 Configuration
            writeRegister(obj,obj.REG_RXFnSIDH(5), bitand(bitshift(filter5,-21),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnSIDL(5), bitand(bitor(bitor(bitshift(bitand(bitshift(filter5,-18),3),5),obj.FLAG_EXIDE),bitand(bitshift(filter5,-16),3)),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID8(5), bitand(bitshift(filter5,-8),255), 'uint8');
            writeRegister(obj,obj.REG_RXFnEID0(5), bitand(filter5,255), 'uint8');
        end
        
        function flag = isInactivePropertyImpl(obj,propertyName)
            switch propertyName
                case {'messageId','identifierType','msgLength','outputError','outputRemote'}
                    if strcmp(obj.outputDataType,'Raw data')
                        flag = false;
                    else
                        flag = true;
                    end
                otherwise
                    flag = isInactivePropertyImpl@codertarget.raspi.internal.SPIMasterTransfer(obj,propertyName);
            end
        end
        
        function num = getNumOutputsImpl(obj)
            num = 2;
            if strcmp(obj.outputDataType,'Raw data')
                if obj.outputError
                    num = num + 1;
                end
                if obj.outputRemote
                    num = num + 1;
                end
            end
        end
        
        function flag = isOutputSizeLockedImpl(~,~)
            flag = true;
        end
        
        function varargout = isOutputComplexImpl(obj)
            for N = 1 : obj.getNumOutputsImpl
                varargout{N} = false;
            end
        end
        
        function varargout = isOutputFixedSizeImpl(obj)
            for N = 1 : obj.getNumOutputsImpl
                varargout{N} = true;
            end
        end
        
        function out = getNumInputsImpl(~)
            out = 0;
        end
        
        function varargout = getOutputSizeImpl(obj)
            N = 1;
            if strcmp(obj.outputDataType,'Raw data')
                varargout{N} = [1 obj.msgLength];           % Raw data
                N = N + 1;
                varargout{N} = [1 1];           % Status
                N = N + 1;
                if obj.outputError
                    varargout{N} = [1 1];
                    N = N + 1;
                end
                if obj.outputRemote
                    varargout{N} = [1 1];
                end
            else
                varargout{N} = [1 1];           % CAN Msg
                N = N + 1;
                varargout{N} = [1 1];           % Status
            end
        end
        
        function varargout = getOutputDataTypeImpl(obj)
            if strcmp(obj.outputDataType,'Raw data')
                N = 1;
                varargout{N} = 'uint8';             %Raw data
                N = N + 1;
                varargout{N} = 'uint8';             % Status
                N = N + 1;
                
                if obj.outputError
                    varargout{N} = 'uint8';
                    N = N + 1;
                end
                if obj.outputRemote
                    varargout{N} = 'uint8';
                end
            else
                varargout{1} = "Bus: raspiCANMsg";
                varargout{2} = "uint8";
            end
        end
        
        function maskDisplayCmds = getMaskDisplayImpl(obj)
            outport_label = [];
            
            maskDisplayCmds = [ ...
                ['color(''white'');', newline]...                                     % Fix min and max x,y co-ordinates for autoscale mask units
                ['plot([100,100,100,100],[100,100,100,100]);',newline]...
                ['plot([0,0,0,0],[0,0,0,0]);',newline]...
                ['color(''blue'');',newline] ...                                     % Drawing mask layout of the block
                ['text(99, 92, ''' obj.Logo ''', ''horizontalAlignment'', ''right'');',newline] ...
                ['color(''black'');',newline] ...
                ['text(50,35,''SPI: ' num2str(obj.SPIModule) ''' ,''horizontalAlignment'', ''center'');', newline], ...
                ['text(50,15,''Chip select: ' num2str(obj.SSPin) ''' ,''horizontalAlignment'', ''center'');', newline], ...
                ];
            
            if strcmp(obj.outputDataType,'Raw data')
                IDStr = ['Message ID: ',num2str(obj.messageId)];
                maskDisplayCmds = [maskDisplayCmds,...
                    ['text(50,75,''\fontsize{12}\bfCAN Receive'',''texmode'',''on'',''horizontalAlignment'',''center'',''verticalAlignment'',''middle'');',newline], ...
                    ['text(50,55,''\fontsize{10}' IDStr ''',''texmode'',''on'',''horizontalAlignment'',''center'',''verticalAlignment'',''middle'');',newline] ...
                    ];
            else
                maskDisplayCmds = [maskDisplayCmds,...
                    ['text(50,50,''\fontsize{12}\bfCAN Receive'',''texmode'',''on'',''horizontalAlignment'',''center'',''verticalAlignment'',''middle'');',newline], ...
                    ];
            end
            
            num = getNumOutputsImpl(obj);
            if num > 1
                outputs = cell(1,num);
                [outputs{1:num}] = getOutputNamesImpl(obj);
                for i = 1:num
                    outport_label = [outport_label 'port_label(''output'',' num2str(i) ',''' outputs{i} ''');' newline]; %#ok<AGROW>
                end
            end
            
            maskDisplayCmds = [maskDisplayCmds,...
                outport_label, ...
                ];
        end
        
        function varargout = getOutputNamesImpl(obj)
            % Return output port names for System block
            N = 1;
            if strcmp(obj.outputDataType,'Raw data')
                varargout{N} = 'Data';
                N = N + 1;
                varargout{N} = 'Status';
                N = N + 1;
                if obj.outputError
                    varargout{N} = 'Error';
                    N = N + 1;
                end
                if obj.outputRemote
                    varargout{N} = 'Remote';
                end
            else
                varargout{N} = 'CAN Msg';
                N = N + 1;
                varargout{N} = 'Status';
            end
        end
    end
    
    
    methods(Static, Access=protected)
        function header = getHeaderImpl()
            header = matlab.system.display.Header(mfilename('class'),...
                'ShowSourceLink', false, ...
                'Title','CAN Receive', ...
                'Text', ['Receive CAN message from MCP2515.', newline, newline ...
                'In "Raw data" mode, the block outputs values received as [1xN] array of type uint8', newline ...
                'In "CAN Msg" mode, the block outputs Simulink bus signal.', newline ...
                'To extract data from Simulink bus signal, connect it to CAN Unpack block from Vehicle Network Toolbox', newline]);
        end
        
        function sts = getSampleTimeImpl(obj)
            sts = getSampleTimeImpl@matlabshared.svd.BlockSampleTime(obj);
        end
        
        function groups = getPropertyGroupsImpl
            % Define section for properties in System block dialog box.
            OutputDataTypeProp = matlab.system.display.internal.Property(...
                'outputDataType', 'Description', 'Data to be output as');
            IDProp = matlab.system.display.internal.Property(...
                'messageId', 'Description', 'Message ID');
            IdentifierTypeProp = matlab.system.display.internal.Property(...
                'identifierType', 'Description', 'Identifier Type');
            MessageLengthProp = matlab.system.display.internal.Property(...
                'msgLength', 'Description', 'Message Length');
            SampleTimeProp = matlab.system.display.internal.Property(...
                'SampleTime', 'Description', 'Sample Time');
            
            
            OutputErrorProp = matlab.system.display.internal.Property(...
                'outputError', 'Description', 'Output Error');
            OutputRemoteProp = matlab.system.display.internal.Property(...
                'outputRemote', 'Description', 'Output Remote');
            
           [~, SPIProp] = codertarget.raspi.internal.SPIMasterTransfer.getPropertyGroupsImpl;
            
            spiGroup = matlab.system.display.Section (...
                'Title', 'SPI Module and CS Selection','PropertyList',...
                SPIProp);
            paramGroup = matlab.system.display.Section(...
                'Title', 'Parameters', 'PropertyList', ...
                {OutputDataTypeProp,IdentifierTypeProp,IDProp,MessageLengthProp,SampleTimeProp});
            
            outputPortsGroup = matlab.system.display.Section(...
                'Title', 'OutputPorts', 'PropertyList', ...
                {OutputErrorProp,OutputRemoteProp});
            
            groups = [spiGroup, paramGroup,outputPortsGroup];
        end
    end
    
    methods(Static)
        function updateBuildInfo(buildInfo, context)
            updateBuildInfo@codertarget.raspi.internal.SPIMasterTransfer(buildInfo, context);
            if context.isCodeGenTarget('rtw')
                % MCP2515 CAN Source file
                spkgrootDir = codertarget.raspi.internal.getSpPkgRootDir;
                addIncludePaths(buildInfo, fullfile(spkgrootDir,'include'));
                buildInfo.addIncludeFiles('MW_MCP2515_CAN.h');
                addSourcePaths(buildInfo, fullfile(spkgrootDir,'src'));
                addSourceFiles(buildInfo,'MW_MCP2515_CAN.c', fullfile(spkgrootDir,'src'), 'BlockModules');
                systemTargetFile = get_param(buildInfo.ModelName,'SystemTargetFile');
                if isequal(systemTargetFile,'ert.tlc')
                    addLinkFlags(buildInfo,'-lpigpio');
                    addLinkFlags(buildInfo,'-lrt');
                end
            end
        end
        
    end
    
end
