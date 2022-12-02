function changeRPiSPIChannel(desChannel)
% Function to modify the default SPI channel on the RaspberryPi

% Check to make sure the desired channel is integer valued
if(floor(desChannel) ~= desChannel)
    error('Desired channel must be integer valued')
end
if(desChannel < 0)
    error('Desired channel out of range')
end
originalDir = pwd;
cd C:\ProgramData\MATLAB\;
possibleDir = dir('**/MW_SPI.c');
for i = 1:length(possibleDir)
    if contains(possibleDir(i).folder, 'rasp') == 1
       SPIDir = possibleDir(i).folder;
    end
end
cd (SPIDir);
fid = fopen('MW_SPI.c','r');
f=fread(fid,'*char')';
fclose(fid);
notificationChange = count(f,"/dev/spidev");
desiredText = ['/dev/spidev' num2str(desChannel) '.'];
f = regexprep(f,'/dev/spidev..',desiredText);
fid  = fopen('MW_SPI.c','w');
fprintf(fid,'%s',f);
fclose(fid);
cd(originalDir)
disp("Successfully made " + notificationChange + " changes to SPI Config.")