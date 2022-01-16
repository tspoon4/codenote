enum Enum { Identifier, Text(String) }
struct Class { member: f32 }

trait Interface
{
	fn pure_method(&self) -> f32;
	fn default_method(&self) -> f32 { return 0.0; }
}

impl Class
{
	fn static_function() -> Class { return Class { member: 0.0 } }
	fn const_method(&self) -> f32 { return self.member; }
	fn mutable_method(&mut self, v: f32) { self.member = v; }
}

impl Interface for Class
{
	fn pure_method(&self) -> f32 { return self.member; }
}

impl Drop for Class
{
	fn drop(&mut self) { println!("Class dropped!") }
}

fn main()
{
	// Stack allocation
	let _x: f32 = 2.0;
	let mut a: [i32; 5] = [1, 2, 3, 4, 5]; a[2] = 12;
	println!("{}", a[2]);

	// Heap allocation
	let mut class: Box<Class> = Box::new(Class::static_function());

	// Scoped borrow
	{
		let borrow: &Box<Class> = &class;
		borrow.const_method();
		borrow.default_method();
		println!("{}", borrow.member);
	}

	// Modify instance after borrow disappear
	class.pure_method();
	class.mutable_method(1.0);
	println!("{}", class.member);

	// Closure
	let closure_inc = |i: f32| -> f32 { i + 1.0 };

	// String and Vec important data structures
	let mut strg = String::from("Hello"); strg.push_str(" world!");
	let mut vec: Vec<Class> = Vec::new(); vec.push(Class::static_function());

	// Main flow control syntax
	for item in vec.iter()
	{
		if item.member > 2.0 { println!("{}", closure_inc(item.member)); }
		else { println!("too small!"); }
	}
}

