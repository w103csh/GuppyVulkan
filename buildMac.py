
import argparse, os, subprocess, sys
import xml.etree.ElementTree as ET

########################
# Check environmental variables
########################

schemeEnvVars = {
    'VK_ICD_FILENAMES' : '',
    'VK_LAYER_PATH' : '',
}

for key, value in schemeEnvVars.items():
    if key not in os.environ:
        print('%s enivronmental variable required but not found.' % key)
        sys.exit()
    else:
        schemeEnvVars[key] = os.environ[key]

########################
# Run CMake
########################

cmd = ['cmake', '-GXcode']
cmd.extend(sys.argv[1:])

proc = subprocess.Popen(cmd, stderr=subprocess.PIPE)

stdoutdata, stderrdata = proc.communicate()

if proc.returncode is not 0:
    print('Error: '  + stderrdata.decode('ascii'))
    sys.exit()

########################
# Find scheme and do xml parse and replace
########################

print('')
projectRoot = 'GuppyVulkan'
projectName = 'Guppy'
schemePath = './%s.xcodeproj/xcshareddata/xcschemes/%s.xcscheme' % (projectRoot, projectName)
schemeExists = os.path.isfile(schemePath)
if not schemeExists:
    print('Could find scheme: %s\nEnvironmental variables not set!\n' % schemePath)
    sys.exit()
else:
    print('Updating scheme: %s\n' % schemePath)

tree = ET.parse(schemePath)
root = tree.getroot()

# Element names. Who knows if these change often
launchActionStr = 'LaunchAction'
environmentVariablesStr = 'EnvironmentVariables'
environmentVariableStr = 'EnvironmentVariable'

launchActionEle = root.find(launchActionStr)
environmentVariablesEle = root.find(environmentVariablesStr)

if environmentVariablesEle is None:
    environmentVariablesEle = ET.Element(environmentVariablesStr)
    launchActionEle.insert(0, environmentVariablesEle)

for key, value in schemeEnvVars.items():
    element = ET.Element(environmentVariableStr, attrib={
        'key': key,
        'value': value,
        'isEnabled': "YES"
    })
    environmentVariablesEle.insert(0, element)

tree.write(schemePath)