#ifndef KERNEL_FUNCTIONS_HPP
#define KERNEL_FUNCTIONS_HPP

#include <Windows.h>
#include <winternl.h>

__kernel_entry NTSTATUS NtQueryInformationThread(
	[ in ]            HANDLE          ThreadHandle,
	[ in ]            THREADINFOCLASS ThreadInformationClass,
	[ in, out ]       PVOID           ThreadInformation,
	[ in ]            ULONG           ThreadInformationLength,
	[ out, optional ] PULONG          ReturnLength
);

#endif