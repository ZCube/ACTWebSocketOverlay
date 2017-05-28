## python3 required.
## from https://stackoverflow.com/questions/5710867/python-downloading-and-unzipping-a-zip-file-without-writing-to-disk/5711095#5711095

from io import BytesIO
from urllib.request import urlopen
from zipfile import ZipFile
import shutil
import os

#libressl not worked with websocket now.
# if not (os.path.exists("libressl/32") and  os.path.exists("libressl/64")):
	# # libressl
	# zipurl = "https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-2.5.4-windows.zip"
	# with urlopen(zipurl) as z:
			# with ZipFile(BytesIO(z.read())) as zfile:
					# zfile.extractall('.')
	# shutil.move("libressl-2.5.4-windows", "libressl")
	# shutil.move("libressl/x86", "libressl/32")
	# shutil.move("libressl/x64", "libressl/64")

if not os.path.exists("openssl"):
		if not os.path.exists("openssl"):
				os.makedirs("openssl")

# openssl 32
if not os.path.exists("openssl/32"):
	zipurl = "https://github.com/ZCube/openssl_build_script/releases/download/openssl-1.0.2l/openssl-1.0.2l-32bit-release-DLL-VS2017.zip"
	with urlopen(zipurl) as z:
			with ZipFile(BytesIO(z.read())) as zfile:
					zfile.extractall('.')
	shutil.move("openssl-1.0.2l-32bit-release-DLL-VS2017", "openssl/32")
	
# openssl 64
if not os.path.exists("openssl/64"):
	zipurl = "https://github.com/ZCube/openssl_build_script/releases/download/openssl-1.0.2l/openssl-1.0.2l-64bit-release-DLL-VS2017.zip"
	with urlopen(zipurl) as z:
			with ZipFile(BytesIO(z.read())) as zfile:
					zfile.extractall('.')
	shutil.move("openssl-1.0.2l-64bit-release-DLL-VS2017", "openssl/64")
