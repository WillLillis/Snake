#pragma once
// Here's how I get pthreads.h to properly include in Visual Studio
// I think I already had the git tool installed on my machine, so you're on your own for that
// After that, I followed the online docuemntation for vcpkg: https://vcpkg.io/en/getting-started.html
	// Command: git clone https://github.com/Microsoft/vcpkg.git
	// Command: .\vcpkg\bootstrap-vcpkg.bat
	// Add vcpkg to system PATH variable
	// Command: vcpkg install pthreads:x64-windows
	// Command: vcpkg integrate install 
		// Requires elevated privileges
// At this point, the project built just fine, although I did have to close and reopen Visual Studio 
// for the IntelliSense warnings to go away
