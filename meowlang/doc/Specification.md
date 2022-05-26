# Meow Language specification

This is the specification for the Meow Language. (Subject to change - this is not stable.)

## Functions

Function declarations can be as follows:

```meow
func name(x y z) {
  ...
}
```

or

```meow
λ (x y z) {
  ...
}

// alternatively...

lambda (x y z) {
  ...
}
```

The latter is a lambda function. Unlike many languages, Meow allows naming of lambda functions as follows:

```meow
λ meow (x y z) {
  ...
}
```

This is a "syntactic sugar" feature of the language. Internally, they function similarly to lambdas in other languages.

The return type can be specified with an arrow operator: "->".

```meow
func name(x y z) -> string {
  ...
}

func name(x y z) -> integer {
  ...
}

lambda (x y z) -> boolean {
  ...
}
```

## Variables and Types

Variables are created like:

```meow
var name : type;
var name : type = value;
```

There is some degree of type inference. However, certain types aren't inferred:

- Numerical literals will always infer as Double instead of Integer.

Meow has a very basic type system. As of the current version of the language there are:

- Integers (note the lack of inference)

  ```meow
  var myInteger : integer;
  var myInteger : integer = 10;
  ```

- Strings

  ```meow
  var myString : string;
  var myString = "Hello, world";
  ```

- Booleans

  ```meow
  var myBoolean : boolean;
  var myBoolean = true;
  ```

- Doubles (or Floats)

  ```meow
  var myDouble : double;
  var myDouble = 10.5;
  ```

