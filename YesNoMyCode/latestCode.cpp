#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h> 
#include <conio.h>
#define framesize 300
#define MaxFrameCount 400 //Maximum we can handle these many frames
#define Ampscale 5000
#define SpeechLength 600000//Maximum number of samples that can be processed
#define IgnoreSamples 600 //we need to ignore initial few samples
#define samplingrate 16000
#define InitFrames 15

using namespace std;

short samples[SpeechLength], MaxAmp=0;
float ThresholdZCR, DCshift=0;
double TotalEnergy=0, ThresholdEnergy, TotalZCR=0; //TotalZCR is only for first few InitFrames
long start=0, stop=0, framecount, samplecount=0; //start and end marker for speech signal YES or NO
string line, filename;

ifstream InpSpeech;
ofstream ScaledInpSpeech;

int main()
{
		long i,j;
	//Try to open the file
		cout << "Enter the file name:  ";
    	getline(cin, filename);
    	InpSpeech.open(filename.c_str(), ios::in);
		if (!InpSpeech)
        {
    		cout << "\n**File failed to open**\n\n";
            InpSpeech.clear();
        }

	//count the number of samples and frames in the file
		if (InpSpeech.is_open())
		{
			while ( !InpSpeech.eof() )
			{ 
				getline (InpSpeech,line);
				samplecount+=1;     
				if(samplecount > IgnoreSamples + 4) //first 4 lines in text file indiactes type of encoding of the speech
				{
					samples[samplecount - (IgnoreSamples + 5)] = (short)atoi(line.c_str()); //5=4+1 4-->indicates encoding, 1-->array index starts with 0
					DCshift+=samples[samplecount - (IgnoreSamples + 5)];
					if(abs(samples[samplecount - (IgnoreSamples + 5)])>MaxAmp)
						MaxAmp=abs(samples[samplecount - (IgnoreSamples + 5)]);
				}
			}
			InpSpeech.close();
		}
		samplecount = samplecount - (IgnoreSamples + 4);
		framecount = samplecount/framesize;
		DCshift=DCshift/samplecount;
		cout << "Number of Samples = " << samplecount << "\n";
		cout << "Number of frames = " << framecount << "\n";
		cout << "DC shift needed is " << DCshift << "\n";
		cout << "Maximum Amplitude = " << MaxAmp << "\n";

	//Store scaled samples in different file
		ScaledInpSpeech.open("ScaledInpSpeech.txt");
		for(i=0;i<samplecount;i++)
			ScaledInpSpeech << (samples[i] - DCshift)*Ampscale/MaxAmp << "\n";
		ScaledInpSpeech.close();
	//use the scaled samples
		InpSpeech.open("ScaledInpSpeech.txt", ios::in);
		if (!InpSpeech)
        {
    		cout << "\n**File failed to open**\n\n";
            InpSpeech.clear();
        }
		InpSpeech.close();
	//ZCR and Energy calculation
		float AvgZCR[MaxFrameCount], AvgEnergy[MaxFrameCount];
		for(i=0;i<framecount;i++)
		{
			AvgZCR[i]=0;
			AvgEnergy[i]=0;
			for(j=0;j<framesize-1;j++)
			{
				if((samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp*((samples[i*framesize + j + 1]-DCshift)*Ampscale*1.0 < 0))
					AvgZCR[i]+=1;
				AvgEnergy[i]+=1.0*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp;
			}
			//cout << "ZCR in "<<i+1<<"th frame is "<< AvgZCR[i] << "\n";
			AvgZCR[i]/=framesize;
			//cout << "Avergae ZCR in "<<i+1<<"th frame is "<< AvgZCR[i] << "\n";
			AvgEnergy[i]+=1.0*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp*(samples[i*framesize + j]-DCshift)*Ampscale/MaxAmp;
			//cout << "Energy in "<<i+1<<"th frame is "<< AvgEnergy[i] << "\n";
			AvgEnergy[i]/=framesize;
			//cout << "Average Energy in "<<i+1<<"th frame is "<< AvgEnergy[i] << "\n";
			TotalEnergy+=AvgEnergy[i];
		}
		for(i=0;i<InitFrames;i++)
			TotalZCR+=AvgZCR[i];
		ThresholdZCR=TotalZCR/InitFrames;
		ThresholdZCR*=0.9;
		ThresholdEnergy=TotalEnergy/framecount; //threshold energy is the average of all averaged energies
		ThresholdEnergy*=0.9;
		bool flag=false;
	//start and end marker of speech
		for(i=0;i<framecount-3;i++)
		{
			//if(AvgEnergy[i+1]>ThresholdEnergy)
			if(AvgZCR[i+1]<ThresholdZCR || AvgEnergy[i+1]>ThresholdEnergy)
			{
				if(flag == false && AvgEnergy[i+2]>ThresholdEnergy && AvgEnergy[i+3]>ThresholdEnergy)
				{
					start = i  ; //i th frame is the starting frame marker
					cout << "Starting frame is "<< start+1 <<"th frame and starting sample is "<< (start+1)*framesize <<"\n";
					cout << "Starting time = " << 1.0*(start + 1)*framesize/samplingrate << " seconds\n" ;
					flag = true;
				}
			}
			//if(flag == true && i > 3) // start is marked, so we need to mark end now
			else if(flag == true && AvgZCR[i] > ThresholdZCR && AvgEnergy[i] < ThresholdEnergy && AvgEnergy[i-1] < ThresholdEnergy)
			{
				//if(AvgEnergy[i]<ThresholdEnergy && AvgEnergy[i-1]<ThresholdEnergy && AvgEnergy[i-2]>ThresholdEnergy)
				//{
					stop = i ;
					cout << "Ending frame is "<< stop+1 <<"th frame and Ending sample is "<< (stop+1)*framesize<<"\n";
					cout << "Ending time = " << 1.0*(stop + 1)*framesize/samplingrate << " seconds\n" ;
					flag = false;
				//}
			}
		}
	
		double testavg=0;
		for(i=start;i<=stop;i++)
			testavg+=AvgEnergy[i];
		testavg/=(stop-start+1);
		float lcount=0, rcount=0;
		bool tflag=false;
		for(i=start;i<=stop;i++)
			if(tflag==false && AvgEnergy[i] < testavg)
			{
				lcount++;
				if(AvgEnergy[i+1] > testavg)
					tflag=true;
			}
			else if(tflag==true&&AvgEnergy[i] < testavg)
				rcount++;
		cout << "lcount = "<<lcount << endl;
		cout << "rcount = "<<rcount << endl;
		//if(rcount>3.9*lcount)
		if(rcount>3*lcount) //to work with mohit sir voice + huge noise
			cout << "speech might be YES signal\n";
		else
			cout << "speech might be NO signal\n";

	//Detect it as Yes or No
		long mark;
		mark=(stop+start)*3/4;
		if(mark == 0)
			cout << "Non speech signal\n";
/*		else if(AvgZCR[mark]>ThresholdZCR && AvgEnergy[mark]>ThresholdEnergy)
			cout << "speech might be YES signal\n";
		else
			cout << "speech might be NO signal\n";
*/	
	getch();
	return 0;
}