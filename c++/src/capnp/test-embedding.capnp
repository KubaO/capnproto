
@0xe5bd84cbdbe2f890;

struct Outer {
	embed0 @0 <0> :Middle;
}

struct Middle {
	middle0 @0 :Inner;
}

struct Inner {
	inner0 @0 :UInt32;
}
