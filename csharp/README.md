# C#

## Reverse engineering

### Tools
* [ILSpy](https://github.com/icsharpcode/AvaloniaILSpy)
* Mono disassembler: **monodis**
* Mono assembler: **ilasm**

### Decompilation
* ILSpy load assembly
* ILSpy decompile: Rclick on assembly -> Save Code... -> C# Single File

### Disassemble & reassemble
Note currently there is a bug in the **monodis** output where hex tokens miss ' '  
In the disassembled.il add the ' ' around tokens before assembling
> monodis --output=disassembled.il assembly.dll  
> ilasm /dll /output:assembly2.dll disassembled.il  

