import fileinput
import json
import re

ffp = r'E:\Jonathan\Arduino\libraries\Audio\gui\index.html'
typeL = ["Synth", "Effect", "Filter", "Mixer", "Amp", "Analyze"]
pStart = r'data-container-name="NodeDefinitions"'
pEnd = r'</script>'
pTypes = r'Audio(' + '|'.join(typeL) + ')'

objmap = {
    	"AudioAnalyzeFFT1024": {"label": "ff1k" },
	"AudioAnalyzeFFT256": {"label": "f256" },
	"AudioAnalyzeNoteFrequency": {"label": "nfrq" },
	"AudioAnalyzePeak": {"label": "peak" },
	"AudioAnalyzePrint": {"label": "prnt" },
	"AudioAnalyzeRMS": {"label": "rms" },
	"AudioAnalyzeToneDetect": {"label": "tone" },
	"AudioEffectBitcrusher": {"label": "crsh" },
	"AudioEffectChorus": {"label": "chor" },
	"AudioEffectDelay": {"label": "dely" },
	"AudioEffectDelayExternal": {"label": "delX" },
	"AudioEffectDigitalCombine": {"label": "cmbn" },
	"AudioEffectEnvelope": {"label": "envL" },
	"AudioEffectExpEnvelope": {"label": "envE" },
	"AudioEffectFade": {"label": "fade" },
	"AudioEffectFlange": {"label": "flng" },
	"AudioEffectFreeverb": {"label": "frvb" },
	"AudioEffectFreeverbStereo": {"label": "frvS" },
	"AudioEffectGranular": {"label": "gran" },
	"AudioEffectMidSide": {"label": "mdsd" },
	"AudioEffectMultiply": {"label": "mply" },
	"AudioEffectRectifier": {"label": "rect" },
	"AudioEffectReverb": {"label": "rvrb" },
	"AudioEffectWaveFolder": {"label": "fold" },
	"AudioEffectWaveshaper": {"label": "shpr" },
	"AudioFilterBiquad": {"label": "biqd" },
	"AudioFilterFIR": {"label": "fir" },
	"AudioFilterLadder": {"label": "ladr" },
	"AudioFilterStateVariable": {"label": "svf" },
	"AudioAmplifier": {"label": "amp" },
	"AudioMixer": {"label": "mixN", "new": "(8)", "inputs": 8},
	"AudioMixer4": {"label": "mix4" },
	"AudioMixerStereo": {"label": "mixS" , "new": "(8)", "inputs": 8},
	"AudioSynthKarplusStrong": {"label": "kpst" },
	"AudioSynthNoisePink": {"label": "npnk" },
	"AudioSynthNoiseWhite": {"label": "nwht" },
	"AudioSynthSimpleDrum": {"label": "drum" },
	"AudioSynthToneSweep": {"label": "tswp" },
	"AudioSynthWaveform": {"label": "wavf" },
	"AudioSynthWaveformDc": {"label": "dc" },
	"AudioSynthWaveformModulated": {"label": "wvmd" },
	"AudioSynthWaveformPWM": {"label": "wpwm" },
	"AudioSynthWaveformSine": {"label": "wsin" },
	"AudioSynthWaveformSineHires": {"label": "wshr" },
	"AudioSynthWaveformSineModulated": {"label": "wsmd" },
	"AudioSynthWavetable": {"label": "wtab" },
}

x = {'type': 'AudioFilterFIR',
     'data': {'defaults': {'name': {'value': 'new'}}, 'shortName': 'fir', 'inputs': 1, 'outputs': 1, 'category': 'filter-function', 'color': '#E6E0F8', 'icon': 'arrow-in.png'}}


def condIndex(d,i,rv):
    if i in d:
        rv = d[i]
    return rv        

# read in the object list
fi = fileinput.input(ffp)
state = 'start'
jsonstr = ''
for li in fi:
    if 'read' == state:
        mtch = re.search(pEnd,li)
        if mtch:
            state = 'done'
            break
        else:
            jsonstr += li
        
    if 'start' == state:
        mtch = re.search(pStart,li)
        if mtch:
            state = 'read'
        
fi.close()

objl = json.loads(jsonstr)['nodes']
opd = {}
for obj in objl:
    mtch = re.search(pTypes,obj['type'])
    if mtch:
        nam = obj['type']
        opd[obj['data']['category']+nam] = (obj,nam)

cats = set()
for id in sorted(opd):
    obj,nam = opd[id]
    shrt = re.sub('(Audio|Synth|Effect|Analyze|Filter)','',nam)
    id = re.sub('([A-Z])','_\g<1>',nam)[1:].upper().replace('P_W_M','PWM').replace('F_I_R','FIR')
    id = id.replace('R_M_S','RMS').replace('F_F_T','FFT')
    inc = obj['data']['inputs']
    opc = obj['data']['outputs']
    cat = obj['data']['category'].replace('-function','')
    cats |= set((cat,))
    lab = newc = ""
    try:
        lab = condIndex(objmap[nam],'label',"")
        newc = condIndex(objmap[nam],"new","")
        inc = condIndex(objmap[nam],"inputs",inc)
    except:
        pass
    #print(f"{obj['type']}: {obj['data']['inputs']} inputs, {obj['data']['outputs']} outputs")
    macro = f"\tAUDIO_ENTRY({nam},{shrt},{id},{inc},{opc},{cat},{lab},{newc}) \\"
    internal = f'\t"{nam}": {{"label": "{nam}" }},'

    print(macro)
    #print(internal)

print("\n\n")
acats = sorted(list(map(lambda x:'AudioCategory_'+x,cats)))
acats = ["AudioCategory_none","AudioCategory_patchcord"] + acats
print(f'enum AudioCategory_e {{ {", ".join(acats)} }};')
