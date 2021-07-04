
%  --------------------------------------------------
%  Script for reading the delays of searchBT function
%  --------------------------------------------------

fileID = fopen('timestamps.bin');
timestamps = fread(fileID,'double');
plot(timestamps);
title('Delay of searchBT');
xlabel('Number of search');
ylabel('Delay in seconds');