from subprocess import check_output
import re
import glob
import sys

dev = False

if len(sys.argv) > 1:
  dev = True
out = check_output(["git", "describe", "--long"])
if (sys.version_info > (3, 0)):
  out = str(out, "utf-8")
out = out.replace('-', '.', 1).replace("\r","").replace("\n","")
arr = out.split(".")
var = arr[0:-1]
if len(var) < 3:
  var = var + ['0'] * (3 - len(arr))

if dev:
  vars = var + [arr[-1].split('-')[0]]
  varl = var + [arr[-1].split('-')[0]+'-'+sys.argv[1]]
else:
  vars = var + [arr[-1].split('-')[0]]
  varl = var + [arr[-1]]

def tagOnly():
  return ".".join(var[0:3])

def tagOnlyWithComma():
  return ".".join(var[0:3])

def longVersion():
  return ".".join(varl)

def longVersionWithComma():
  return ",".join(varl)
  
def shortVersion():
  return ".".join(vars)

def shortVersionWithComma():
  return ",".join(vars)
  
print(tagOnly())
print(longVersion())
print(longVersionWithComma())
print(shortVersion())
print(shortVersionWithComma())

files = glob.glob("**/*.in") + glob.glob("*.in")
for f in files:
  print(f[:-3])
  with open(f, 'r') as infile, open(f[:-3], 'w') as outfile:
    ctx = infile.read()
    ctx = ctx.replace("@VERSION_TAG@", tagOnly())
    ctx = ctx.replace("@VERSION_TAG_WITH_COMMA@", tagOnlyWithComma())
    ctx = ctx.replace("@VERSION_SHORT@", shortVersion())
    ctx = ctx.replace("@VERSION_SHORT_WITH_COMMA@", shortVersionWithComma())
    ctx = ctx.replace("@VERSION_LONG@", longVersion())
    ctx = ctx.replace("@VERSION_LONG_WITH_COMMA@", longVersionWithComma())
    outfile.write(ctx)