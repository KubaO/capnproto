
@0xca4d5e511d2da69c;

struct Template!(T1) {
	t1 @0 :T1;
}

struct Subst1 {
	f1 @0 :Template!(UInt64);
}

struct Outer!(T) {
	inner @0 :Inner!(T);
}

struct Inner!(T) {
	outer @0 :Outer!(T);
}

struct Application {
	outer @0 :Outer!(UInt64);
}

# 1st subsitution pass

struct Application1 {
	outer @0 :Outer_UInt64_1;
}

struct Outer_UInt64_1 { 
	inner @0 :Inner!(UInt64);
}

# 2nd substitution pass

struct Outer_UInt64_2 {
	inner @0 :Inner_UInt64_2;
}

struct Inner_UInt64_2
	outer @0 :Outer_UInt64_2;
}

# Done. No recursion.