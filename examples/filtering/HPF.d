/* ------------------------------------------------------------
name: "HPF"
Code generated with Faust 2.28.8 (https://faust.grame.fr)
Compilation options: -lang dlang -scal -ftz 0
------------------------------------------------------------ */
/************************************************************************
 IMPORTANT NOTE : this file contains two clearly delimited sections :
 the ARCHITECTURE section (in two parts) and the USER section. Each section
 is governed by its own copyright and license. Please check individually
 each section for license and copyright information.
 *************************************************************************/

/*******************BEGIN ARCHITECTURE SECTION (part 1/2)****************/

/************************************************************************
 FAUST Architecture File
 Copyright (C) 2003-2019 GRAME, Centre National de Creation Musicale
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.
 
 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.
 
 ************************************************************************
 ************************************************************************/

// faust -a minimal.cpp noise.dsp -o noise.cpp && c++ -std=c++11 noise.cpp -o noise && ./noise

/******************************************************************************
 *******************************************************************************
 
 VECTOR INTRINSICS
 
 *******************************************************************************
 *******************************************************************************/


/********************END ARCHITECTURE SECTION (part 1/2)****************/

/**************************BEGIN USER SECTION **************************/

module mydsp;

import std.math;
import core.stdc.string;


float sinf(float dummy0);
float cosf(float dummy0);

alias FAUSTFLOAT = float;

alias FAUSTCLASS = mydsp;


class mydsp : dsp {
	
 private:
	
	int fSampleRate;
	float fConst0;
	float fHslider0;
	float fHslider1;
	float[] fRec0 = new float[3];
	
 public:
	
	void metadata(Meta* m) { 
		m.declare("filename", "HPF.dsp");
		m.declare("maths.lib/author", "GRAME");
		m.declare("maths.lib/copyright", "GRAME");
		m.declare("maths.lib/license", "LGPL with exception");
		m.declare("maths.lib/name", "Faust Math Library");
		m.declare("maths.lib/version", "2.3");
		m.declare("maxmsp.lib/author", "GRAME");
		m.declare("maxmsp.lib/copyright", "GRAME");
		m.declare("maxmsp.lib/license", "LGPL with exception");
		m.declare("maxmsp.lib/name", "MaxMSP compatibility Library");
		m.declare("maxmsp.lib/version", "1.1");
		m.declare("name", "HPF");
		m.declare("platform.lib/name", "Generic Platform Library");
		m.declare("platform.lib/version", "0.1");
	}

	int getNumInputs() {
		return 1;
	}
	int getNumOutputs() {
		return 1;
	}
	int getInputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	int getOutputRate(int channel) {
		int rate;
		switch ((channel)) {
			case 0: {
				rate = 1;
				break;
			}
			default: {
				rate = -1;
				break;
			}
		}
		return rate;
	}
	
	static void classInit(int sample_rate) {
	}
	
	void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = (6.28318548f / std.math.fmin(192000.0f, std.math.fmax(1.0f, float(fSampleRate))));
	}
	
	void instanceResetUserInterface() {
		fHslider0 = float(1000.0f);
		fHslider1 = float(1.0f);
	}
	
	void instanceClear() {
		for (int l0 = 0; (l0 < 3); l0 = (l0 + 1)) {
			fRec0[l0] = 0.0f;
		}
	}
	
	void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	mydsp* clone() {
		return new mydsp();
	}
	
	int getSampleRate() {
		return fSampleRate;
	}
	
	void buildUserInterface(UI* ui_interface) {
		ui_interface.openVerticalBox("HPF");
		ui_interface.addHorizontalSlider("Freq", &fHslider0, 1000.0f, 100.0f, 10000.0f, 1.0f);
		ui_interface.addHorizontalSlider("Q", &fHslider1, 1.0f, 0.00999999978f, 100.0f, 0.00999999978f);
		ui_interface.closeBox();
	}
	
	void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) {
		float[] input0 = inputs[0][0..count];
		float[] output0 = outputs[0][0..count];
		float fSlow0 = (fConst0 * std.math.fmax(0.0f, float(fHslider0)));
		float fSlow1 = (0.5f * (std.math.sin(fSlow0) / std.math.fmax(0.00100000005f, float(fHslider1))));
		float fSlow2 = (1.0f / (fSlow1 + 1.0f));
		float fSlow3 = std.math.cos(fSlow0);
		float fSlow4 = (-1.0f - fSlow3);
		float fSlow5 = (0.0f - (2.0f * fSlow3));
		float fSlow6 = (1.0f - fSlow1);
		float fSlow7 = (0.5f * (fSlow3 + 1.0f));
		for (int i = 0; (i < count); i = (i + 1)) {
			fRec0[0] = (float(input0[i]) - (fSlow2 * ((fSlow5 * fRec0[1]) + (fSlow6 * fRec0[2]))));
			output0[i] = float((fSlow2 * (((fSlow4 * fRec0[1]) + (fSlow7 * fRec0[0])) + (fSlow7 * fRec0[2]))));
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
		}
	}

}

/***************************END USER SECTION ***************************/

/*******************BEGIN ARCHITECTURE SECTION (part 2/2)***************/
import std.stdio;
import std.conv;

class Meta {
    void declare(string name, string value) {}
}

interface FaustVarAccess {
    string getId();
    void set(float val);
    float get();
}

class UI {

    void declare(string id, string key, string value) {}

    // -- layout groups

    void openTabBox(string label) {}
    void openHorizontalBox(string label) {}
    void openVerticalBox(string label) {}
    void closeBox() {}

    // -- active widgets

    void addButton(string label, FaustVarAccess varAccess) {}
    void addCheckButton(string label, FaustVarAccess varAccess) {}
    void addVerticalSlider(string label, FaustVarAccess varAccess, float init, float min, float max, float step) {}
    void addHorizontalSlider(string label, FaustVarAccess varAccess, float init, float min, float max, float step) {}
    void addNumEntry(string label, FaustVarAccess varAccess, float init, float min, float max, float step) {}

    // -- passive display widgets

    void addHorizontalBargraph(string label, FaustVarAccess varAccess, float min, float max) {}
    void addVerticalBargraph(string label, FaustVarAccess varAccess, float min, float max) {}

}

class dsp {
public:
    int fSamplingFreq;
}

void main(string[] args)
{
    mydsp DSP;
    writeln("DSP size: " ~ to!string(DSP.sizeof) ~ " bytes\n");
    
    // Activate the UI, here that only print the control paths
    // PrintUI ui;
    // DSP.buildUserInterface(&ui);

    // // Allocate the audio driver to render 5 buffers of 512 frames
    // dummyaudio audio(5);
    // audio.init("Test", &DSP);
    
    // // Render buffers...
    // audio.start();
    // audio.stop();
}

/********************END ARCHITECTURE SECTION (part 2/2)****************/

