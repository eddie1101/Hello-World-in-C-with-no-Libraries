// Compile command used:
// gcc -fno-stack-protector -nostdlib -static -o main main.c
//
// -fno-stack-protector was required because the compiler was
// trying to insert chk_stack calls as a security feature to
// defend against stack smashing. Without libc, the chk_stack
// symbols were unidentified.
// 
// -nostdlib prevents libc from being used to wrap main, which
// happens even if no libc code is #include'd.
//
// -static tells the compiler/linker to prefer static libraries
// over runtime ones. This makes the executable size of main 5k
// larger than when it is used (9k vs 14k on my machine). Running
// ldd on the executable emits "Statically linked" when --static
// is omitted, and "Not dynamically linked" when --static is
// used. I'm not sure why, or what the difference is.
//
// Compiled on Arch 6.17.8, GCC 15.2.1, LD 2.45.1

// Based on the content of this youtube video:
// https://www.youtube.com/watch?v=gVaXLlGqQ-c

// _start is the entry point for all programs on Linux
// Normally, libc provides _start and the compiler wraps main in it
// No libc means we need to write _start ourselves
int _start()
{
  // Char array to print, as normal. Can have any content.
  char hello[] = "What is the meaning of life...?\n";

  // Long is important here, because it is 8 bytes, and the variable
  // must be the same size as the register it will be placed in.
  const long length = sizeof(hello);

  // No libc means no printf or puts from stdio.h, and no write
  // wrapper from unistd.h
  // Note that because operands are prefixed by '%', registers are
  // prefixed by '%%' so that the compiler can distinguish
  // operands from registers. Normally in asm, registers are
  // prefixed by a single '%'.
  // Also note that the asm by default must be Gnu Assembler Syntax,
  // or GAS, which is AT&T syntax by default.
  asm volatile (
    // On x86-64, rax holds the number identifying the syscall when
    // the kernel is called. 1 is the number for write.
    "mov $1, %%rax\n"
    // The write syscall looks in rdi for the file descriptor to
    // write to. 1 is the fd for stdout.
    "mov $1, %%rdi\n"
    // The write syscall looks in rsi for the starting address of
    // chars to write. %0 is the first operand to the asm.
    "lea %0, %%rsi\n"
    // The write syscall looks in rdx for the size, or number of
    // chars to write. %1 is the second operand to the asm.
    "mov %1, %%rdx\n"
    // Invoke call to kernel.
    "syscall\n"
    : // No output operands.          // <-- output operands here
    : "m"(hello), "b"(length)         // <-- input operands here
    // The "m" and "b" in the line above are register constraints.
    // They tell the compiler where the operands will be placed in
    // the generated asm. "m" means memory, which makes sense as we
    // are performing an lea (Load Effective Address) on the message
    // which is in memory and will not fit in any register. "b"
    // indicates that the length variable will be in register %ebx.
    // a, b, c, d indicate rax, rbx, rcx, rdx respectively. "r" can
    // be used to mean any available general register.
    : "%rax", "%rdi", "%rsi", "%rdx"  // <-- changed registers here
    // The list of registers above is called the "clobbered"
    // registers and it tells the compiler which registers have
    // their contents changed, and therefore cannot have any
    // assumptions made about them.
  );  

  // The _start provided by libc that wraps main also performs the
  // exit syscall after main finishes executing, passing the return
  // value of main to the syscall as the return value of the program.
  asm volatile (
    // Again, rax holds the indentifier for the syscall. 60 is the
    // number which identifies the exit syscall.
    "mov $60, %%rax\n"
    // The exit syscall looks in rdi for the exit status. 0 means
    // the program finished executing successfully.
    "mov $0, %%rdi\n"
    // Invoke call to kernel.  
    "syscall\n"
    // No outputs or inputs, and only the two clobbered registers.
    ::: "%rax", "%rdi"
  );

  // Not called, but linters complain without a return value.
  return 0;
}
