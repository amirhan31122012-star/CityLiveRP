#include "CityChatInput.h"
#include <string>
#include <cassert>
#include <iostream>
int main(){std::u16string a=u"Привет";assert(CityChatInput::EraseCount(a.data(),a.size())==1);a=u"A\U0001F697";assert(CityChatInput::EraseCount(a.data(),a.size())==2);assert(CityChatInput::Fit(a.data(),a.size(),2)==1);assert(CityChatInput::Fit(a.data(),a.size(),3)==3);assert(CityChatInput::EraseCount(a.data(),0)==0);std::cout<<"PASS: Cyrillic and UTF-16 backspace, safe length boundary\n";}
