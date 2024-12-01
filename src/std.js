//
// This software is dedicated to the public domain.
//

function call(f, ...args)
{
	return f(...args);
}

function include(file)
{
	switch(file == "std") {
	case true: {
	}};
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

function _(txt)
{
	return (new TextEncoder()).encode(txt);
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

function free(o)
{
	delete o;
}

function file_size(f)
{
	return 0;
}

function file_load(f,s)
{
}

