//
// This software is dedicated to the public domain.
//

function _(txt)
{
	return (new TextEncoder()).encode(txt);
}


const RIKIKI = _("JavaScript Version 1.0");

class RIKIKI_CLASS {
	hello;	
}

function call(f, ...args)
{
	return f(...args);
}

async function include(src)
{
	await import(src).then((m) => { 
		for (var f in m) {
			if (typeof globalThis[f] === "undefined") {
				globalThis[f] = m[f];
			} else {
				console.log("#error : " + f + "already exists");
			}
		}
	});
}

function struct(v,cls)
{
}

function array(n)
{
	return new Array(n);
}

function bytes(n)
{
	return new Uint8Array(n);
}

function free(v)
{
	v = null;
}

function poke(b,i,v)
{
	var n = v;
	if (typeof v === "string") {
		n = v.charCodeAt(0);
	}
	b[i] = n;
}

function peek(b,i)
{
	return b[i];
}

function print(v)
{
	var s = (new TextDecoder()).decode(v);
	console.log(s);
}

function print2(v)
{
	console.log(parseInt(v));
}

let fs;
if (typeof process !== 'undefined') {
	fs = await import('fs');
} else {
	fs = 0;
}

function file_size(f)
{
	if (typeof process !== 'undefined') {
		var s = (new TextDecoder()).decode(f);
		const st = fs.statSync(s);
		return st.size;
	}
}

function file_load(f,s)
{
	if (typeof process !== 'undefined') {
		var s = (new TextDecoder()).decode(f);
		return fs.readFileSync(s, null);
	}
}

function file_save(f,s,b)
{
	if (typeof process !== 'undefined') {
		var s = (new TextDecoder()).decode(f);
		return fs.writeFileSync(s, b.subarray(0,s));
	}
}

// FIXME
export function end(m) {
	switch (typeof process !== 'undefined') {
	case true:
		m(2,[_("theApp"), _("../README.md"), 0]);break;
	default:
		m(2,[_("theApp"), _("Yes"), 0]);break;
	}
}

globalThis['_'] = _;
globalThis['struct'] = struct;
globalThis['array'] = array;
globalThis['bytes'] = bytes;
globalThis['call'] = call;
globalThis['poke'] = poke;
globalThis['peek'] = peek;
globalThis['print'] = print;
globalThis['print2'] = print2;
globalThis['free'] = free;
globalThis['file_size'] = file_size;
globalThis['file_save'] = file_save;
globalThis['file_load'] = file_load;
globalThis['include'] = include;
globalThis['end'] = end;
globalThis['RIKIKI'] = RIKIKI;
globalThis['RIKIKI_CLASS'] = RIKIKI_CLASS;
