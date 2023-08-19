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

        "AudioControlAK4558": {"label": "ctrl" },
	"AudioControlCS42448": {"label": "ctrl" },
	"AudioControlCS4272": {"label": "ctrl" },
	"AudioControlSGTL5000": {"label": "ctrl" },
	"AudioControlWM8731": {"label": "ctrl" },
	"AudioControlWM8731master": {"label": "ctrl" },
	"AsyncAudioInputSPDIF3": {"label": "SPDIF" },
	"AudioInputAnalog": {"label": "ANAi" },
	"AudioInputAnalogStereo": {"label": "ANAi" },
	"AudioInputI2S": {"label": "I2Si" },
	"AudioInputI2S2": {"label": "I2Si" },
	"AudioInputI2SHex": {"label": "I2Si" },
	"AudioInputI2SOct": {"label": "I2Si" },
	"AudioInputI2SQuad": {"label": "I2Si" },
	"AudioInputI2Sslave": {"label": "I2Si" },
	"AudioInputPDM": {"label": "PDMi" },
	"AudioInputPDM2": {"label": "PDM2i" },
	"AudioInputSPDIF3": {"label": "SPDIF" },
	"AudioInputTDM": {"label": "TDMi" },
	"AudioInputTDM2": {"label": "TDM2i" },
	"AudioInputUSB": {"label": "USBi" },
	"AudioOutputADAT": {"label": "ADATo" },
	"AudioOutputAnalog": {"label": "ANAo" },
	"AudioOutputAnalogStereo": {"label": "ANAo" },
	"AudioOutputI2S": {"label": "I2So" },
	"AudioOutputI2S2": {"label": "I2So" },
	"AudioOutputI2SHex": {"label": "I2So" },
	"AudioOutputI2SOct": {"label": "I2So" },
	"AudioOutputI2SQuad": {"label": "I2So" },
	"AudioOutputI2Sslave": {"label": "I2So" },
	"AudioOutputMQS": {"label": "MQSo" },
	"AudioOutputPT8211": {"label": "PT8211" },
	"AudioOutputPT8211_2": {"label": "PT8211" },
	"AudioOutputPWM": {"label": "PWMo" },
	"AudioOutputSPDIF": {"label": "SPDIF" },
	"AudioOutputSPDIF2": {"label": "SPDIF" },
	"AudioOutputSPDIF3": {"label": "SPDIF" },
	"AudioOutputTDM": {"label": "TDMo" },
	"AudioOutputTDM2": {"label": "TDM2o" },
	"AudioOutputUSB": {"label": "USBo" },
}

x = {'type': 'AudioFilterFIR',
     'data': {'defaults': {'name': {'value': 'new'}}, 'shortName': 'fir', 'inputs': 1, 'outputs': 1, 'category': 'filter-function', 'color': '#E6E0F8', 'icon': 'arrow-in.png'}}

# Extra underscore fixer-upper:
usl = ["P_W_M","F_I_R","R_M_S","F_F_T","A_K","C_S","S_G_T_L","W_M","S_P_D_I_F","I2_S","P_D_M","T_D_M","U_S_B"]
usd = {}
for u in usl:
    usd[u] = u.replace("_","")
def fixUnderscores(s):
    for u in usd:
        s = s.replace(u,usd[u])
    return s        

    
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
def makeMacros(pTypes):
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
        id = re.sub('([a-z])([A-Z])','\g<1>_\g<2>',nam)[0:].upper()
        #id = fixUnderscores(id)
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

    acats = sorted(list(map(lambda x:'AudioCategory_'+x,cats)))
    
    return acats 

acats = ["AudioCategory_none","AudioCategory_patchcord"]
print("#define AUDIO_ENTRIES \\")
acats+= makeMacros(pTypes)
print("\n\n/*")
acats += makeMacros(r'Audio(' + '|'.join(["Input","Output","Control"]) + ')')
print("*/\n\n")
print(f'enum AudioCategory_e {{ {", ".join(acats)} }};')
           
