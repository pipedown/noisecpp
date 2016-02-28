#![feature(plugin)]
#![plugin(peg_syntax_ext)]


#[derive(Clone, PartialEq, Eq, Debug)]
pub enum Token {
    // The `Key'`s value is always a string
    Key(Box<Token>, Box<Token>),
    Array(Box<Token>),
    And(Box<Token>, Box<Token>),
    Equals(Box<Token>, Box<Token>),
    String(String),
    Not(Box<Token>),
}


peg! noise(r#"
use super::Token;

#[pub]
token -> Token
    = and
and -> Token
    = l:equals "AND" r:and { Token::And(Box::new(l), Box::new(r)) }
    / equals
equals -> Token
    = k:string '=' v:string { Token::Equals(Box::new(k), Box::new(v)) }
    / array
array -> Token
    = '[' t:token ']' { Token::Array(Box::new(t)) }
    / '.' key
    / key
key -> Token
    = k:string a:array { Token::Key(Box::new(k), Box::new(a)) }
    / not
not -> Token
    = "NOT" t:key { Token::Not(Box::new(t)) }
    / string
string -> Token
    = '"' [a-zA-Z0-9]+ '"' { Token::String(match_str.parse().unwrap()) }
"#);


fn main() {
    //let result = noise::token(r#""Akey""#);
    //let result = noise::token(r#""Akey"["B"]"#);
    //let result = noise::token(r#""A"["B"="b1"]"#);
    //let result = noise::token(r#""A"["B"="b1"AND"C"="c1"]"#);
    //let result = noise::token(r#""A"["B"="b1"AND"C"="c1"]"#);
    //let result = noise::token(r#""A"["B"="b1"AND"C"="c1"AND"D"="d1"]"#);
    //let result = noise::token(r#""A"["B"="b1"]AND"A"["C"="c1"]"#);
    //let result = noise::token(r#""A"."B"["C"="c1"]"#);
    //let result = noise::token(r#""A"."B"["C"="c1"AND"D"="d1"]"#);
    //let result = noise::token(
    //    r#""A"."B"["C"="c1"AND"D"="d1"]AND"A"."B"["C"="c1"]AND"A"."B"["D"="d1"]"#);
    //let result = noise::token(r#"NOT"A"["B"="b1"]"#);
    //let result = noise::token(r#"NOT"A"."B"["B"="b1"AND"D"="d1"]"#);
    //let result = noise::token(r#"NOT"A"AND"B""#);
    //let result = noise::token(r#"NOT"A"["B"="b1"]AND"B"["C"="c1"]"#);
    //let result = noise::token(r#"NOT"A"."B"["B"="b1"AND"D"="d1"]AND"A"."B"["C"="c1"]"#);
    let result = noise::token(
        r#"NOT"A"."B"["B"="b1"AND"D"="d1"]AND"A"."B"["C"="c1"]AND"A"."B"["D"="d1"]"#);
    //let result = noise::token(r#""A"."B"["C"="c1"AND"D"="d1"]"#);
    //let result = noise::token(r#""A"AND"B""#);
    //let result = noise::token(r#""B"="b1""#);
    //let result = expression(r#"1+3+4"#);
    println!("result: {:?}", result);
}


#[test]
pub fn noise_tests() {
    assert_eq!(noise::token(r#""A""#), Ok(Token::String(r#""A""#.to_string())));
    assert_eq!(noise::token(r#""A"AND"B""#), Ok(Token::And(
	Box::new(Token::String(r#""A""#.to_string())),
	Box::new(Token::String(r#""B""#.to_string())))));
    assert_eq!(noise::token(r#""B"="b1""#), Ok(Token::Equals(
	Box::new(Token::String(r#""B""#.to_string())),
	Box::new(Token::String(r#""b1""#.to_string())))));
    assert_eq!(noise::token(r#""A"["B"]"#), Ok(Token::Key(
	Box::new(Token::String(r#""A""#.to_string())),
        Box::new(Token::Array(
	    Box::new(Token::String(r#""B""#.to_string())))))));
    assert_eq!(noise::token(r#""A"["B"="b1"]"#), Ok(Token::Key(
	Box::new(Token::String(r#""A""#.to_string())),
        Box::new(Token::Array(
            Box::new(Token::Equals(
	        Box::new(Token::String(r#""B""#.to_string())),
                Box::new(Token::String(r#""b1""#.to_string())))))))));
    assert_eq!(noise::token(r#""A"["B"="b1"AND"C"="c1"]"#), Ok(Token::Key(
	Box::new(Token::String(r#""A""#.to_string())),
        Box::new(Token::Array(
            Box::new(Token::And(
                Box::new(Token::Equals(
	            Box::new(Token::String(r#""B""#.to_string())),
                    Box::new(Token::String(r#""b1""#.to_string())))),
                Box::new(Token::Equals(
	            Box::new(Token::String(r#""C""#.to_string())),
                    Box::new(Token::String(r#""c1""#.to_string())))))))))));
    assert_eq!(noise::token(r#""A"["B"="b1"AND"C"="c1"AND"D"="d1"]"#), Ok(Token::Key(
	Box::new(Token::String(r#""A""#.to_string())),
        Box::new(Token::Array(
            Box::new(Token::And(
                Box::new(Token::Equals(
	            Box::new(Token::String(r#""B""#.to_string())),
                    Box::new(Token::String(r#""b1""#.to_string())))),
                Box::new(Token::And(
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""C""#.to_string())),
                        Box::new(Token::String(r#""c1""#.to_string())))),
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""D""#.to_string())),
                        Box::new(Token::String(r#""d1""#.to_string())))))))))))));
    assert_eq!(noise::token(r#""A"["B"="b1"]AND"A"["C"="c1"]"#), Ok(
        Token::And(
            Box::new(Token::Key(
	        Box::new(Token::String(r#""A""#.to_string())),
                Box::new(Token::Array(
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""B""#.to_string())),
                        Box::new(Token::String(r#""b1""#.to_string())))))))),
            Box::new(Token::Key(
	        Box::new(Token::String(r#""A""#.to_string())),
                Box::new(Token::Array(
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""C""#.to_string())),
                        Box::new(Token::String(r#""c1""#.to_string())))))))))));
    assert_eq!(noise::token(r#""A"."B"["C"="c1"]"#), Ok(Token::Key(
	Box::new(Token::String(r#""A""#.to_string())),
        Box::new(Token::Key(
            Box::new(Token::String(r#""B""#.to_string())),
            Box::new(Token::Array(
                Box::new(Token::Equals(
	            Box::new(Token::String(r#""C""#.to_string())),
                    Box::new(Token::String(r#""c1""#.to_string())))))))))));
    assert_eq!(noise::token(r#""A"."B"."C"["D"="d1"]"#), Ok(Token::Key(
	Box::new(Token::String(r#""A""#.to_string())),
        Box::new(Token::Key(
            Box::new(Token::String(r#""B""#.to_string())),
            Box::new(Token::Key(
                Box::new(Token::String(r#""C""#.to_string())),
                Box::new(Token::Array(
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""D""#.to_string())),
                        Box::new(Token::String(r#""d1""#.to_string())))))))))))));
    assert_eq!(noise::token(r#""A"."B"["C"="c1"AND"D"="d1"]"#), Ok(Token::Key(
	Box::new(Token::String(r#""A""#.to_string())),
        Box::new(Token::Key(
            Box::new(Token::String(r#""B""#.to_string())),
            Box::new(Token::Array(
                Box::new(Token::And(
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""C""#.to_string())),
                        Box::new(Token::String(r#""c1""#.to_string())))),
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""D""#.to_string())),
                        Box::new(Token::String(r#""d1""#.to_string())))))))))))));
    assert_eq!(noise::token(
        r#""A"."B"["C"="c1"AND"D"="d1"]AND"A"."B"["C"="c1"]AND"A"."B"["D"="d1"]"#), Ok(Token::And(
            Box::new(Token::Key(
	        Box::new(Token::String(r#""A""#.to_string())),
                Box::new(Token::Key(
                    Box::new(Token::String(r#""B""#.to_string())),
                    Box::new(Token::Array(
                        Box::new(Token::And(
                            Box::new(Token::Equals(
	                        Box::new(Token::String(r#""C""#.to_string())),
                                Box::new(Token::String(r#""c1""#.to_string())))),
                            Box::new(Token::Equals(
	                        Box::new(Token::String(r#""D""#.to_string())),
                                Box::new(Token::String(r#""d1""#.to_string())))))))))))),
            Box::new(Token::And(
                Box::new(Token::Key(
	            Box::new(Token::String(r#""A""#.to_string())),
                    Box::new(Token::Key(
                        Box::new(Token::String(r#""B""#.to_string())),
                        Box::new(Token::Array(
                            Box::new(Token::Equals(
	                        Box::new(Token::String(r#""C""#.to_string())),
                                Box::new(Token::String(r#""c1""#.to_string())))))))))),
                Box::new(Token::Key(
	            Box::new(Token::String(r#""A""#.to_string())),
                    Box::new(Token::Key(
                        Box::new(Token::String(r#""B""#.to_string())),
                        Box::new(Token::Array(
                            Box::new(Token::Equals(
	                        Box::new(Token::String(r#""D""#.to_string())),
                                Box::new(Token::String(r#""d1""#.to_string())))))))))))))));
    assert_eq!(noise::token(r#"NOT"A"["B"="b1"]"#), Ok(Token::Not(
        Box::new(Token::Key(
	    Box::new(Token::String(r#""A""#.to_string())),
            Box::new(Token::Array(
                Box::new(Token::Equals(
	            Box::new(Token::String(r#""B""#.to_string())),
                    Box::new(Token::String(r#""b1""#.to_string())))))))))));
    assert_eq!(noise::token(r#"NOT"A"."B"["C"="c1"AND"D"="d1"]"#), Ok(Token::Not(
        Box::new(Token::Key(
	    Box::new(Token::String(r#""A""#.to_string())),
            Box::new(Token::Key(
                Box::new(Token::String(r#""B""#.to_string())),
                Box::new(Token::Array(
                    Box::new(Token::And(
                        Box::new(Token::Equals(
	                    Box::new(Token::String(r#""C""#.to_string())),
                            Box::new(Token::String(r#""c1""#.to_string())))),
                        Box::new(Token::Equals(
                            Box::new(Token::String(r#""D""#.to_string())),
                            Box::new(Token::String(r#""d1""#.to_string())))))))))))))));
    assert_eq!(noise::token(r#"NOT"A"AND"B""#), Ok(Token::And(
        Box::new(Token::Not(
            Box::new(Token::String(r#""A""#.to_string())))),
        Box::new(Token::String(r#""B""#.to_string())))));
    assert_eq!(noise::token(r#"NOT"A"["B"="b1"]AND"B"["C"="c1"]"#), Ok(Token::And(
        Box::new(Token::Not(
            Box::new(Token::Key(
	        Box::new(Token::String(r#""A""#.to_string())),
                Box::new(Token::Array(
                    Box::new(Token::Equals(
	                Box::new(Token::String(r#""B""#.to_string())),
                        Box::new(Token::String(r#""b1""#.to_string())))))))))),
        Box::new(Token::Key(
	    Box::new(Token::String(r#""B""#.to_string())),
            Box::new(Token::Array(
                Box::new(Token::Equals(
	            Box::new(Token::String(r#""C""#.to_string())),
                    Box::new(Token::String(r#""c1""#.to_string())))))))))));

    assert_eq!(noise::token(
        r#"NOT"A"."B"["C"="c1"AND"D"="d1"]AND"A"."B"["C"="c1"]"#), Ok(Token::And(
            Box::new(Token::Not(
                Box::new(Token::Key(
	            Box::new(Token::String(r#""A""#.to_string())),
                    Box::new(Token::Key(
                        Box::new(Token::String(r#""B""#.to_string())),
                        Box::new(Token::Array(
                            Box::new(Token::And(
                                Box::new(Token::Equals(
	                            Box::new(Token::String(r#""C""#.to_string())),
                                    Box::new(Token::String(r#""c1""#.to_string())))),
                                Box::new(Token::Equals(
                                    Box::new(Token::String(r#""D""#.to_string())),
                                    Box::new(Token::String(r#""d1""#.to_string())))))))))))))),
            Box::new(Token::Key(
	        Box::new(Token::String(r#""A""#.to_string())),
                Box::new(Token::Key(
                    Box::new(Token::String(r#""B""#.to_string())),
                    Box::new(Token::Array(
                        Box::new(Token::Equals(
	                    Box::new(Token::String(r#""C""#.to_string())),
                            Box::new(Token::String(r#""c1""#.to_string())))))))))))));

    assert_eq!(noise::token(
        r#"NOT"A"."B"["C"="c1"AND"D"="d1"]AND"A"."B"["C"="c1"]AND"A"."B"["D"="d1"]"#), Ok(Token::And(
            Box::new(Token::Not(
                Box::new(Token::Key(
	            Box::new(Token::String(r#""A""#.to_string())),
                    Box::new(Token::Key(
                        Box::new(Token::String(r#""B""#.to_string())),
                        Box::new(Token::Array(
                            Box::new(Token::And(
                                Box::new(Token::Equals(
	                            Box::new(Token::String(r#""C""#.to_string())),
                                    Box::new(Token::String(r#""c1""#.to_string())))),
                                Box::new(Token::Equals(
                                    Box::new(Token::String(r#""D""#.to_string())),
                                    Box::new(Token::String(r#""d1""#.to_string())))))))))))))),
	    Box::new(Token::And(
		Box::new(Token::Key(
		    Box::new(Token::String(r#""A""#.to_string())),
		    Box::new(Token::Key(
			Box::new(Token::String(r#""B""#.to_string())),
			Box::new(Token::Array(
			    Box::new(Token::Equals(
				Box::new(Token::String(r#""C""#.to_string())),
				Box::new(Token::String(r#""c1""#.to_string())))))))))),

		Box::new(Token::Key(
		    Box::new(Token::String(r#""A""#.to_string())),
		    Box::new(Token::Key(
			Box::new(Token::String(r#""B""#.to_string())),
			Box::new(Token::Array(
			    Box::new(Token::Equals(
				Box::new(Token::String(r#""D""#.to_string())),
				Box::new(Token::String(r#""d1""#.to_string())))))))))))))));
}
