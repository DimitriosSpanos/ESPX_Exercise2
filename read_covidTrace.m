
%  --------------------------------------------------
%    Script for reading the uploaded close contacts
%  --------------------------------------------------

fileID = fopen('covidTrace.bin');
covidTrace = fread(fileID,'uint64');