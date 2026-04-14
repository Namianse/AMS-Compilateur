//  A compiler from a very simple Pascal-like structured language LL(k)
//  to 64-bit 80x86 Assembly langage
//  Copyright (C) 2019 Pierre Jourlin
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Build with "make compilateur"


#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;

char current;				// Current car
char nextcar;				// Next car
string terminal;			// Detected terminal

void ReadChar(void){
	do(
		current = nextcar;
		cin.get(nextcar);
	)while(current != EOF && (nextcar==' ' || nextcar=='\t' || nextcar=='\n'));
}

void Error(string s){
	cerr<< s << endl;
	exit(-1);
}

// ArithmeticExpression := Term {AdditiveOperator Term}
// Term := Digit | "(" ArithmeticExpression ")"
// AdditiveOperator := "+" | "-"
// Digit := "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"

string SingleTerminal(void){
	terminal = current;
	ReadChar();
	return(terminal);
}

string DoubleTerminal(void){
	terminal = current + nextcar;
	ReadChar();
	ReadChar();
	return(terminal);
}

string AdditiveOperator(void){
	if(current=='+'|| current=='-'){
		return(SingleTerminal());
	}
	else if(current=='|' && nextcar=='|') {
		return(DoubleTerminal());
	}
	else
		Error("Opérateur additif attendu");	   // Additive operator expected
}

string MultiplicativeOperator(void){
	if(current=='*'||current=='/'||current=='%'){
		return(SingleTerminal());
	}
	else if(current=='&' && nextcar=='&'){
		return(DoubleTerminal());
	}
	else
		Error("Opérateur multiplicatif attendu");	// Multiplicative operator expected

}

string RelationalOperator(void){
	if(current=='=' && nextcar=='='){
		return(DoubleTerminal());
	}
	else if(current=='<' || current=='>'){
		if(nextcar=='='){
			return(DoubleTerminal());
		}
		else {
			return(SingleTerminal());
		}
	}
	else if(current=='!' && nextcar=='='){
		return(DoubleTerminal());
	}
	else{
		Error("Opérateur relationnel attendu");		// Relational operator expected
	}
}
		
string Digit(void){
	if((current<'0')||(current>'9'))
		Error("Chiffre attendu");		   // Digit expected
	else{
		cout << "\tpush $"<<current<<endl;
		return(SingleTerminal());
	}
}

string Number(void){
	string value;
	value = Digit();

	while((current >= '0') && (current <= '9')){
		value += Digit();
	}

	cout << "\tpush $" << value << endl;
	return value;
}

string Letter(void){
	if(!isalpha(current))
		Error("Lettre attendue");		   // Letter expected
	else{
		cout << "\tpush " << current << endl;
		return(SingleTerminal());
	}
}

string Factor(void){
	if(isdigit(current))
		return(Number());
	else if(isalpha(current))
		return(Letter());
	else if(current == '!'){
		ReadChar();
		return(Factor());
	}
	else if(current == '('){
		terminal = Expression();
		if(current != ')'){
			Error("Une expression doit avoir un ) après.");
		}
		else{
			return(terminal);
		}
	}
	else{
		Error("Facteur invalide.")
	}
}

string Term(void){
    Factor();

    while(current=='*'||current=='/'||current=='%' || (current=='&' && nextcar=='&')){
        string op = MultiplicativeOperator();

        Factor();

        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;

        if(op == "*")
            cout << "\timulq %rbx, %rax" << endl;
        else if(op == "/"){
            cout << "\tcqto" << endl;
            cout << "\tidivq %rbx" << endl;
        }
        else if(op == "%"){
            cout << "\tcqto" << endl;
            cout << "\tidivq %rbx" << endl;
            cout << "\tmovq %rdx, %rax" << endl;
        }
        else if(op == "&&"){
            cout << "\tandq %rbx, %rax" << endl;
        }

        cout << "\tpush %rax" << endl;
    }

    return "";
}

string SimpleExpression(void){
    Term();

    while(current=='+'|| current=='-' || (current=='|' && nextcar=='|')){
        string op = AdditiveOperator();

        Term();

        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;

        if(op == "+")
            cout << "\taddq %rbx, %rax" << endl;
        else if(op == "-")
            cout << "\tsubq %rbx, %rax" << endl;
        else if(op == "||")
            cout << "\torq %rbx, %rax" << endl;

        cout << "\tpush %rax" << endl;
    }

    return "";
}

string Expression(void){
	terminal = SimpleExpression();

	if(current == '<' || current == '>' || 
		(nextcar == '=' && 
			(current == '=' || current == '!'))){
		terminal += RelationalOperator();
		terminal += SimpleExpression();
	}

	return terminal;
}

string AssignementStatement(void){
    string var = Letter(); // nom variable

    if(current != '=')
        Error("= attendu");

    ReadChar();

    Expression();

    cout << "\tpop " << var << endl;

    return "";
}

string Statement(void){
	terminal = AssignementStatement();

	return(terminal);
}

string StatementPart(void){
	terminal = Statement();

	while(current == ';'){
		terminal += ';';
		ReadChar();
		terminal += Statement();
	}
	if(current != '.')
		Error("Un . est attendu pour conclure le StatementPart");
	else{
		terminal += '.';
		ReadChar();
		return(terminal);
	}
}

string DeclarationPart(void){
    if(current != '[')
        Error("[");

    cout << ".data" << endl;

    ReadChar();

    string var = Letter();
    cout << var << ":\t.quad 0" << endl;

    while(current == ','){
        ReadChar();
        var = Letter();
        cout << var << ":\t.quad 0" << endl;
    }

    if(current != ']')
        Error("]");

    ReadChar();
    return "";
}

string Program(void){
	if(current == '['){
		DeclarationPart();
	}
	StatementPart();
}

int main(void){	// First version : Source code on standard input and assembly code on standard output
	// Header for gcc assembler / linker
	cout << "\t\t\t# This code was produced by the CERI Compiler"<<endl;
	cout << "\t.text\t\t# The following lines contain the program"<<endl;
	cout << "\t.globl main\t# The main function must be visible from outside"<<endl;
	cout << "main:\t\t\t# The main function body :"<<endl;
	cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top"<<endl;

	// Let's proceed to the analysis and code production
	ReadChar();
	ReadChar();
	ArithmeticExpression();
	ReadChar();
	// Trailer for the gcc assembler / linker
	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top"<<endl;
	cout << "\tret\t\t\t# Return from main function"<<endl;
	if(cin.get(current)){
		cerr <<"Caractères en trop à la fin du programme : ["<<current<<"]";
		Error("."); // unexpected characters at the end of program
	}

}
		
			





