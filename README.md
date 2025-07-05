# xv6-Projects

## About
Assignments from the Operating Systems course, Spring 2025.
<br>
<br>

### Task 1. Implement System Calls
Create five new system calls for the xv6 kernel:
* `getnice(pid)`: Retrieve the nice value (priority) of a given process.
* `setnice(pid, value)`: Set the nice value, controlling scheduling priority (valid range: 0–39, default 20).
* `ps(pid)`: Print process info (name, pid, state, nice value) for a given PID or all processes if pid == 0.
* `meminfo()`: Return available memory in bytes.
* `waitpid(pid)`: Block the current process until the specified process exits.
> 🔍 Refer to the `Make-System-Calls` branch for full implementation.
<br>

### Task 2. Implement EEVDF CPU Scheduler
Replace xv6's default round-robin scheduler with a Linux-like EEVDF (Earliest Eligible Virtual Deadline First) scheduler:
* Schedule the task with the earliest virtual deadline among eligible ones.
* Accurately compute and update vruntime, vdeadline, eligibility, and runtime per tick.
* Modify `ps()` to display: runtime/weight, runtime, vruntime, vdeadline, eligibility, and total_tick.
* Ensure correct handling of forked/woken-up processes.
> 🔍 Refer to commits on the `Linux-EEVDF-Scheduler` branch starting from Apr 9, 2025.
<br>

### Task 3. Implement mmap(), munmap(), freemem() and Page Fault Handler
Introduce virtual memory operations and support for memory-mapped files:
* `mmap(addr, length, prot, flags, fd, offset)`: Map files or anonymous memory to a process’s virtual address space. Supports MAP_ANONYMOUS, MAP_POPULATE, PROT_READ, and PROT_WRITE.
* Page Fault Handler: On access to an unmapped page in a mapping region, allocate a physical page and set up the mapping (with file reading if needed).
* `munmap(addr)`: Unmap a previously mapped region and free associated resources.
* `freemem()`: Return the number of free physical pages in the system.
> 🔍 Refer to commits on the `Page-Fault-Handler` branch starting from May 8, 2025.
<br>

### Task 4. Implement Page Replacement with Swapping
Enable swapping in xv6 to handle memory pressure using a clock algorithm and LRU list:
* Swap out pages to disk when physical memory is full, using `swapwrite()`.
* Swap in pages upon access, using `swapread()`.
* Track swappable pages via a circular LRU list.
* Integrate a bitmap-based swap space tracking mechanism.
* Ensure both swapped-in and swapped-out pages are managed during fork and deallocation.
> 🔍 Refer to commits on the `Page-Replacement` branch starting from May 23, 2025.
<br>

> @SKKU (Sungkyunkwan University)
