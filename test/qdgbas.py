# -*- coding: utf-8 -*-
# Quick and Dirty Game Boy Assembler
import re
import sys
import collections

class InputError(RuntimeError): pass
Op = collections.namedtuple('Op', ['string', 'code', 'size', 'assemble'])

def mkrestr(s):
	return re.sub(r'\s+', r'\s*', re.sub(r'([,)(+])', r' \\\1 ', s))

imm8  = r'[0-9a-fA-F][0-9a-fA-F]'
imm16 = imm8 + imm8
labelregexp = re.compile('\w+\s+(\w+)')

def assembleNormal(outdata, outpos, indata, inpos, targets, op):
	pass

def assembleImm8(outdata, outpos, indata, inpos, targets, op):
	m = re.match(mkrestr(op.string).replace('imm8', '('+imm8+')'), indata[inpos])
	outdata[outpos] = int(m.group(1), 0x10)

def assembleImm16(outdata, outpos, indata, inpos, targets, op):
	m = re.match(mkrestr(op.string).replace('imm16', '('+imm16+')'), indata[inpos])
	outdata[outpos  ] = int(m.group(1)[2:4], 0x10)
	outdata[outpos+1] = int(m.group(1)[0:2], 0x10)

def assembleJr(outdata, outpos, indata, inpos, targets, op):
	label = labelregexp.match(indata[inpos]).group(1)
	t = targets.get(label)
	t = t - (outpos + 1) if t != None else int(label, 0x10)
	
	if t < -0x80 or t > 0x7f:
		raise InputError('line ' + str(i) + ': ' + indata[inpos], 'Relative jump target too far away')
	
	outdata[outpos] = t & 0xff

def assembleJp(outdata, outpos, indata, inpos, targets, op):
	label = labelregexp.match(indata[inpos]).group(1)
	t = targets.get(label)
	
	if t == None:
		t = int(label, 0x10)
	elif t >= 0x8000:
		t = 0x4000 | (t & 0x3fff)
	
	outdata[outpos  ] = t & 0xFF
	outdata[outpos+1] = t >> 8

def makeoplist():
	l = []
	
	def addop(opstr, opcode):
		size = (opcode >= 0) + (opcode >= 0x100)
		moperand = re.match(r'(.*imm8)|(jr.* target)|(.*imm16)|(call.* target|jp.* target)', opstr)
		if moperand:
			optype = [(0, assembleNormal), (1, assembleImm8), (1, assembleJr), (2, assembleImm16), (2, assembleJp)][moperand.lastindex]
			l.append(Op(opstr, opcode, size + optype[0], optype[1]))
		else:
			l.append(Op(opstr, opcode, size, assembleNormal))

	addop('nop', 0x00)
	addop('ld bc, imm16', 0x01)
	addop('ld(bc), a', 0x02)
	addop('inc bc', 0x03)
	addop('inc b', 0x04)
	addop('dec b', 0x05)
	addop('ld b, imm8', 0x06)
	addop('ld a, (bc)', 0x0a)
	addop('inc c', 0x0c)
	addop('dec c', 0x0d)
	addop('ld c, imm8', 0x0e)
	addop('rrca', 0x0f)
	addop('stop, imm8', 0x10)
	addop('ld de, imm16', 0x11)
	addop('ld(de), a', 0x12)
	addop('inc d', 0x14)
	addop('dec d', 0x15)
	addop('ld d, imm8', 0x16)
	addop('jr target', 0x18)
	addop('add hl, de', 0x19)
	addop('ld a, (de)', 0x1a)
	addop('inc e', 0x1c)
	addop('dec e', 0x1d)
	addop('ld e, imm8', 0x1e)
	addop('jrnz target', 0x20)
	addop('ld hl, imm16', 0x21)
	addop('ld(hl++), a', 0x22)
	addop('inc h', 0x24)
	addop('dec h', 0x25)
	addop('ld a, (hl++)', 0x2a)
	addop('inc l', 0x2c)
	addop('dec l', 0x2d)
	addop('ld sp, imm16', 0x31)
	addop('ld(hl--), a', 0x32)
	addop('ld(hl), imm8', 0x36)
	addop('add hl, sp', 0x39)
	addop('ld a, (hl--)', 0x3a)
	addop('inc a', 0x3c)
	addop('dec a', 0x3d)
	addop('ld a, imm8', 0x3e)
	addop('ld b, a', 0x47)
	addop('ld c, a', 0x4f)
	addop('ld d, a', 0x57)
	addop('ld e, a', 0x5f)
	addop('halt', 0x76)
	addop('ld(hl), a', 0x77)
	addop('ld a, b', 0x78)
	addop('ld a, c', 0x79)
	addop('ld a, d', 0x7a)
	addop('ld a, e', 0x7b)
	addop('ld a, h', 0x7c)
	addop('ld a, l', 0x7d)
	addop('ld a, (hl)', 0x7e)
	addop('add a, b', 0x80)
	addop('add a, e', 0x83)
	addop('sub a, b', 0x90)
	addop('sub a, c', 0x91)
	addop('sub a, d', 0x92)
	addop('and a, b', 0xa0)
	addop('and a, c', 0xa1)
	addop('and a, d', 0xa2)
	addop('xor a, a', 0xaf)
	addop('or a, a', 0xb7)
	addop('cmp a, b', 0xb8)
	addop('cmp a, c', 0xb9)
	addop('cmp a, d', 0xba)
	addop('cmp a, e', 0xbb)
	addop('cmp a, h', 0xbc)
	addop('retnz', 0xc0)
	addop('pop bc', 0xc1)
	addop('jpnz target', 0xc2)
	addop('jp target', 0xc3)
	addop('push bc', 0xc5)
	addop('rst 00', 0xc7)
	addop('ret', 0xc9)
	addop('call target', 0xcd)
	addop('pop de', 0xd1)
	addop('push de', 0xd5)
	addop('sub a, imm8', 0xd6)
	addop('reti', 0xd9)
	addop('ldff(imm8), a', 0xe0)
	addop('pop hl', 0xe1)
	addop('ldff(c), a', 0xe2)
	addop('push hl', 0xe5)
	addop('and a, imm8', 0xe6)
	addop('ld(imm16), a', 0xea)
	addop('xor a, imm8', 0xee)
	addop('ldff a, (imm8)', 0xf0)
	addop('pop af', 0xf1)
	addop('ldff a, (c)', 0xf2)
	addop('di', 0xf3)
	addop('push af', 0xf5)
	addop('ld a, (imm16)', 0xfa)
	addop('ei', 0xfb)
	addop('cmp a, imm8', 0xfe)
	addop('sra a', 0xcb2f)
	addop('swap a', 0xcb37)
	addop('srl a', 0xcb3f)
	addop('\w* : ', -1)
	
	return l

def makeopregexp(oplist):
	rxps = ''
	for e in oplist:
		rxps += '(' + mkrestr(e.string) + '$)|'
	return re.compile(rxps[0:len(rxps)-1].replace('imm8', imm8).replace('imm16', imm16).replace('target', r'\s\w+'))

oplist = makeoplist()
opregexp = makeopregexp(oplist)

def maptargets(targets, indata, instart, addr):
	for i in xrange(instart, len(indata)):
		match = opregexp.match(indata[i])
		if match == None:
			break
		
		op = oplist[match.lastindex - 1]
		
		if op.size == 0:
			targets[re.match('(\w*)\s*:', indata[i]).group(1)] = addr
			
		addr += op.size
	
	return i

def astext(outdata, addr, indata, instart, targets):
	for i in xrange(instart, len(indata)):
		match = opregexp.match(indata[i])
		if match == None:
			break
		
		op = oplist[match.lastindex - 1]
		outpos = addr
		
		if op.code >= 0x100:
			outdata[outpos  ] = op.code >> 8
			outdata[outpos+1] = op.code & 0xff
			outpos += 2
		elif op.code >= 0:
			outdata[outpos] = op.code
			outpos += 1
		
		op.assemble(outdata, outpos, indata, i, targets, op)
			
		addr += op.size
	
	return i

def asdata(outdata, addr, indata, instart):
	try:
		for i in xrange(instart, len(indata)):
			ints = [int(x, 0x10) for x in indata[i].split()]
			outdata[addr:addr+len(ints)] = ints
			addr += len(ints)
	except ValueError:
		pass
	
	return i

def putgblogo(outdata):
	outdata[0x104:0x134] = \
		[0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
		 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d,
		 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
		 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99,
		 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
		 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e]

def putheadercsum(outdata):
	outdata[0x14d] = sum([-(x+1) for x in outdata[0x134:0x14d]]) & 0xFF

def putromcsum(outdata):
	csum = sum(outdata)
	outdata[0x14e:0x150] = [csum >> 8 & 0xff, csum & 0xff]

def assembleFile(indata):
	size = 0x8000
	startpos = 0

	for i in xrange(0, len(indata)):
		spl = indata[i].split()
		if len(spl) > 0 and spl[0] == '.size':
			size = int(spl[1], 0x10)
			startpos = i + 1
			break

	targets = {}
	i = startpos
	while i < len(indata):
		spl = [x.strip() for x in indata[i].split('@')]
		i += 1;
		
		if spl[0] == '.text':
			i = maptargets(targets, indata, i, int(spl[1], 0x10))

	outdata = bytearray(size)
	i = startpos
	while i < len(indata):
		if indata[i] != '':
			spl = [x.strip() for x in indata[i].split('@')]
			
			if spl[0] == '.text':
				i = astext(outdata, int(spl[1], 0x10), indata, i+1, targets)
			elif spl[0] == '.data':
				i = asdata(outdata, int(spl[1], 0x10), indata, i+1)
			else:
				raise RuntimeError('line ' + str(i) + ': ' + indata[i])
		else:
			i += 1


	putgblogo(outdata)
	putheadercsum(outdata)
	putromcsum(outdata)
	
	return outdata

def readDataFromFile(filename):
	infile = open(filename, 'r')
	indata = [line.strip() for line in infile]
	infile.close()
	return indata

def writeDataToFile(filename, data):
	outfile = open(filename, 'wb')
	outfile.write(data)
	outfile.close()

def outFilenameFromInFilename(inname, h143):
	return inname.rsplit('.', 1)[0] + ('.gbc' if (h143 & 0x80) else '.gb')

def main():
	for arg in sys.argv[1:]:
		print 'processing ' + arg
		outdata = assembleFile(readDataFromFile(arg))
		writeDataToFile(outFilenameFromInFilename(arg, outdata[0x143]), outdata)
	
	print "\nassembled " + str(len(sys.argv) - 1) + (' files' if len(sys.argv) != 2 else ' file')

if __name__ == "__main__":
	main()
