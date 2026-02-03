// BUILD.zig
//   by Youssef Abdelkareem (ywmaa)
//
// Created:
//   2025.04.14 -20:32
// Last edited:
//   2025.06.02 -05:01
// Auto updated?
//   Yes
//
// Description:
//   Build Script
//

const std = @import("std");
const builtin = @import("builtin");
const android = @import("android");

fn generate_version_string(allocator: std.mem.Allocator, major: i32, minor: i32) ![]u8 {
    // Get current time as timespec
    const now: i64 = std.time.milliTimestamp();

    // Convert milliseconds to seconds
    const seconds = @divTrunc(now, 1000);

    return std.fmt.allocPrint(allocator, "{d}.{d} {d}", .{ major, minor, seconds });
}

pub fn build(b: *std.Build) !void {
    const exe_name: []const u8 = "testbed";
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const android_targets = android.standardTargets(b, target);
    const is_web = target.result.os.tag == .emscripten;
    if (is_web) {
        std.debug.print("Building for the web...\n", .{});
    }

    const major = 0;
    const minor = 1;

    const version_str = generate_version_string(b.allocator, major, minor) catch unreachable;

    // Write to version.h
    const out_path = "engine/src/version.h";
    const file = try std.fs.cwd().createFile(out_path, .{ .truncate = true });
    defer file.close();
    var buf: [1024]u8 = undefined;
    var file_writer = file.writer(&buf);

    // Access the new interface part
    const writer = &file_writer.interface;

    try writer.print(
        \\#pragma once
        \\
        \\#define YVERSION "{s}"
        \\
    , .{version_str});
    try writer.flush(); // flush to ensure buffered data is written

    // Print it in the build output
    std.log.info("Generated version: {s}", .{version_str});

    // If building with Android, initialize the tools / build
    const android_apk: ?*android.Apk = blk: {
        if (android_targets.len == 0) {
            break :blk null;
        }
        const android_sdk = android.Sdk.create(b, .{});
        const apk = android_sdk.createApk(.{
            .api_level = .android15,
            .build_tools_version = "35.0.0",
            .ndk_version = "28.0.13004108", //"29.0.13113456",//"27.0.12077973",
        });

        const key_store_file = android_sdk.createKeyStore(.example);
        apk.setKeyStore(key_store_file);
        apk.setAndroidManifest(b.path("android/AndroidManifest.xml"));
        apk.addResourceDirectory(b.path("android/res"));

        // Add Java files
        apk.addJavaSourceFile(.{ .file = b.path("android/src/NativeInvocationHandler.java") });
        break :blk apk;
    };

    const is_debug = optimize == .Debug;

    var engine_flags = std.array_list.Managed([]const u8).init(b.allocator);
    defer engine_flags.deinit();

    try engine_flags.append("-DYEXPORT");
    if (is_debug) {
        try engine_flags.append("-D_DEBUG");
    }
    try engine_flags.append("-Wall");
    try engine_flags.append("-Wpedantic");
    try engine_flags.append("-Werror");
    try engine_flags.append("-Wno-gnu");
    try engine_flags.append("-Wno-missing-braces");

    const testbed_flags = [_][]const u8{
        "-DYIMPORT",
    };
    // Engine Flag -DYEXPORT
    // Testbed Flag -DYIMPORT

    const libengine = if (is_web) blk: {
        // 1. Create a copy of the target query and add features
        var web_target = target.query;
        const wasm_features = std.Target.wasm.featureSet(&.{ .atomics, .bulk_memory });
        web_target.cpu_features_add = wasm_features;

        break :blk b.addLibrary(.{
            .name = "engine",
            .linkage = .static,
            .root_module = b.createModule(.{
                .target = b.resolveTargetQuery(web_target),
                .optimize = optimize,
                .root_source_file = b.path("testbed/src/main.zig"),
            }),
        });
    } else b.addLibrary(.{
        .name = "engine",
        .linkage = .dynamic, // make it a shared library
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });
    libengine.addIncludePath(.{ .cwd_relative = "engine/src" });
    if (target.result.os.tag == .linux) {
        libengine.addIncludePath(.{ .cwd_relative = "engine/thirdparty/linuxbsd_headers" });
        libengine.addIncludePath(.{ .cwd_relative = "engine/thirdparty/linuxbsd_headers/wayland" });
    }

    // Search for all C files in `src` and add them
    {
        var dir = try std.fs.cwd().openDir("engine/src", .{ .iterate = true });

        var walker = try dir.walk(b.allocator);
        defer walker.deinit();

        const allowed_exts = [_][]const u8{ ".c", ".m" };
        while (try walker.next()) |entry| {
            const ext = std.fs.path.extension(entry.basename);
            const include_file = for (allowed_exts) |e| {
                if (std.mem.eql(u8, ext, e))
                    break true;
            } else false;
            if (include_file) {
                std.debug.print("Engine: Found {s} file to compile: '{s}'. path: '{s}'\n", .{ ext, entry.basename, entry.path });
                libengine.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "engine/src", entry.path })), .flags = engine_flags.items });
            }
        }
    }
    if (target.result.os.tag == .macos) {
        libengine.linkFramework("Foundation");
        libengine.linkFramework("Cocoa");
        libengine.linkFramework("QuartzCore");
    }
    if (!is_web) {
        libengine.root_module.addRPathSpecial("$ORIGIN");
        libengine.root_module.addRPathSpecial("$ORIGIN/lib");
        libengine.root_module.addRPathSpecial("$ORIGIN/../lib");
        libengine.root_module.addRPathSpecial(".");
    }

    libengine.linkLibC();

    var exe: *std.Build.Step.Compile = if (target.result.abi.isAndroid()) b.addLibrary(.{
        .name = exe_name,
        .linkage = .dynamic, // make it a shared library
        .root_module = b.createModule(.{
            .root_source_file = b.path("testbed/src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    }) else b.addExecutable(.{
        .name = exe_name,
        .root_module = b.createModule(.{
            .root_source_file = b.path("testbed/src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    if (is_web) {
        libengine.addIncludePath(.{ .cwd_relative = "testbed/src" });
    } else {
        exe.addIncludePath(.{ .cwd_relative = "engine/src" });
        exe.addIncludePath(.{ .cwd_relative = "testbed/src" });
    }
    // Search for all C files in `src` and add them
    {
        var dir = try std.fs.cwd().openDir("testbed/src", .{ .iterate = true });

        var walker = try dir.walk(b.allocator);
        defer walker.deinit();

        const allowed_exts = [_][]const u8{".c"};
        while (try walker.next()) |entry| {
            const ext = std.fs.path.extension(entry.basename);
            const include_file = for (allowed_exts) |e| {
                if (std.mem.eql(u8, ext, e))
                    break true;
            } else false;
            if (include_file) {
                std.debug.print("Testbed: Found {s} file to compile: '{s}'. path: '{s}'\n", .{ ext, entry.basename, entry.path });
                if (is_web) {
                    libengine.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "testbed/src", entry.path })), .flags = &testbed_flags });
                } else {
                    exe.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "testbed/src", entry.path })), .flags = &testbed_flags });
                }
            }
        }
    }

    //START ************ GAME LIB ************//
    const testbed_lib = b.addLibrary(.{ //addStaticLibrary/addSharedLibrary
        .linkage = .dynamic, // make it a shared library
        .name = "testbed_lib",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });
    if (is_web) {
        libengine.addIncludePath(.{ .cwd_relative = "testbed_lib/src" });
    } else {
        // we are using stuff from the engine
        testbed_lib.addIncludePath(.{ .cwd_relative = "engine/src" });
        testbed_lib.addIncludePath(.{ .cwd_relative = "testbed_lib/src" });
        testbed_lib.linkLibrary(libengine);
    }
    {
        var dir = try std.fs.cwd().openDir("testbed_lib/src", .{ .iterate = true });

        var walker = try dir.walk(b.allocator);
        defer walker.deinit();

        const allowed_exts = [_][]const u8{".c"};
        while (try walker.next()) |entry| {
            const ext = std.fs.path.extension(entry.basename);
            const include_file = for (allowed_exts) |e| {
                if (std.mem.eql(u8, ext, e))
                    break true;
            } else false;
            if (include_file) {
                std.debug.print("Testbed Lib: Found {s} file to compile: '{s}'. path: '{s}'\n", .{ ext, entry.basename, entry.path });
                if (is_web) {
                    libengine.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "testbed_lib/src", entry.path })), .flags = engine_flags.items });
                } else {
                    testbed_lib.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "testbed_lib/src", entry.path })), .flags = engine_flags.items });
                }
            }
        }
    }
    //END ************ GAME LIB ************//

    //START ************ WEBGPU RENDERER PLUGIN ************//
    const webgpu_plugin = b.addLibrary(.{ //addStaticLibrary/addSharedLibrary
        .linkage = .dynamic, // make it a shared library
        .name = "webgpu_renderer",
        .root_module = b.createModule(.{ .target = target, .optimize = optimize }),
    });
    if (is_web) {
        libengine.addIncludePath(.{ .cwd_relative = "webgpu_renderer/src" });
    } else {
        // we are using stuff from the engine
        webgpu_plugin.addIncludePath(.{ .cwd_relative = "engine/src" });
        webgpu_plugin.addIncludePath(.{ .cwd_relative = "webgpu_renderer/src" });
        webgpu_plugin.addIncludePath(.{ .cwd_relative = "engine/thirdparty/linuxbsd_headers" });
        webgpu_plugin.addIncludePath(.{ .cwd_relative = "engine/thirdparty/linuxbsd_headers/wayland" });
        webgpu_plugin.linkLibC();
    }
    {
        var dir = try std.fs.cwd().openDir("webgpu_renderer/src", .{ .iterate = true });

        var walker = try dir.walk(b.allocator);
        defer walker.deinit();

        const allowed_exts = [_][]const u8{".c"};
        while (try walker.next()) |entry| {
            const ext = std.fs.path.extension(entry.basename);
            const include_file = for (allowed_exts) |e| {
                if (std.mem.eql(u8, ext, e))
                    break true;
            } else false;
            if (include_file) {
                std.debug.print("Webgpu Renderer Plugin: Found {s} file to compile: '{s}'. path: '{s}'\n", .{ ext, entry.basename, entry.path });
                if (is_web) {
                    libengine.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "webgpu_renderer/src", entry.path })), .flags = engine_flags.items });
                } else {
                    webgpu_plugin.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "webgpu_renderer/src", entry.path })), .flags = engine_flags.items });
                }
            }
        }
    }
    //In The Future I Should replace this with compiling WGPU from source to be able to statically link WGPU
    var lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin");
    if (target.result.os.tag == .linux) {
        if (target.result.cpu.arch == .aarch64) {
            if (target.result.abi == .android) {
                lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/linux-aarch64");
            } else {
                lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/linux-aarch64");
            }
        } else if (target.result.cpu.arch == .x86_64) {
            if (target.result.abi == .android) {
                lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/linux-x86_64");
            } else {
                lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/linux-x86_64");
            }
        }
    } else if (target.result.os.tag == .windows) {
        if (target.result.cpu.arch == .aarch64) {
            lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/windows-i686-msvc");
        } else if (target.result.cpu.arch == .x86_64) {
            lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/windows-x86_64-msvc");
        }
    } else if (target.result.os.tag == .macos) {
        if (target.result.cpu.arch == .aarch64) {
            lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/macos-aarch64");
        } else if (target.result.cpu.arch == .x86_64) {
            lib_file = b.path("webgpu_renderer/thirdparty/wgpu/bin/macos-x86_64");
        }
    }

    if (target.result.os.tag == .windows) {
        std.debug.print("Webgpu Renderer Plugin: Dynamic Linking {s}\n", .{lib_file.getPath(b)});
        b.installDirectory(.{
            .source_dir = lib_file,
            .install_dir = .prefix,
            .install_subdir = "bin/",
            .include_extensions = &.{".dll"},
        });
        webgpu_plugin.addLibraryPath(lib_file);
        webgpu_plugin.linkSystemLibrary2("wgpu_native.dll", .{
            .use_pkg_config = .no,
            .preferred_link_mode = .dynamic,
        });
        //libengine.linkSystemLibrary2("wgpu_native", .{
        //    .use_pkg_config = .no,
        //    .preferred_link_mode = .static,
        //});
    } else if (!is_web) {
        //b.installDirectory(.{
        //    .source_dir = lib_file,
        //    .install_dir = .prefix,
        //    .install_subdir = "lib/",
        //    .include_extensions = &.{ ".dylib", ".so" },
        //});
        webgpu_plugin.addLibraryPath(lib_file);
        webgpu_plugin.linkSystemLibrary2("wgpu_native", .{
            .use_pkg_config = .no,
            .preferred_link_mode = .dynamic,
        });
    }
    if (!is_web) {
        webgpu_plugin.linkLibrary(libengine);
    }
    //END ************ WEBGPU RENDERER PLUGIN ************//

    if (!is_web) {
        //START ************ VULKAN RENDERER PLUGIN ************//
        const vulkan_plugin = b.addLibrary(.{ //addStaticLibrary/addSharedLibrary
            .linkage = .dynamic, // make it a shared library
            .name = "vulkan_renderer",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
            }),
        });
        // we are using stuff from the engine
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "engine/src" });
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "vulkan_renderer/src" });
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "engine/thirdparty/linuxbsd_headers" });
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "engine/thirdparty/linuxbsd_headers/wayland" });
        // Vullkan Headers extracted from: https://github.com/hexops/vulkan-headers
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "vulkan_renderer/thirdparty/vulkan_headers" });
        {
            var dir = try std.fs.cwd().openDir("vulkan_renderer/src", .{ .iterate = true });

            var walker = try dir.walk(b.allocator);
            defer walker.deinit();

            const allowed_exts = [_][]const u8{".c"};
            while (try walker.next()) |entry| {
                const ext = std.fs.path.extension(entry.basename);
                const include_file = for (allowed_exts) |e| {
                    if (std.mem.eql(u8, ext, e))
                        break true;
                } else false;
                if (include_file) {
                    std.debug.print("Vulkan Renderer Plugin: Found {s} file to compile: '{s}'. path: '{s}'\n", .{ ext, entry.basename, entry.path });
                    vulkan_plugin.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "vulkan_renderer/src", entry.path })), .flags = engine_flags.items });
                }
            }
        }
        vulkan_plugin.linkLibC();

        const glslang_lib = b.addLibrary(.{
            .name = "glslang_compiler",
            .linkage = .dynamic, // make it a shared library
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
            }),
        });
        glslang_lib.linkLibCpp();
        glslang_lib.linkLibC();
        glslang_lib.addIncludePath(.{ .cwd_relative = "vulkan_renderer/thirdparty/glslang" });
        glslang_lib.addIncludePath(.{ .cwd_relative = "vulkan_renderer/thirdparty/glslang/glslang" });
        glslang_lib.addIncludePath(.{ .cwd_relative = "vulkan_renderer/thirdparty/glslang/SPIRV" });
        glslang_lib.addCSourceFiles(.{
            .files = &[_][]const u8{
                //"thirdparty/glslang/glslang/Include/glslang_c_interface.h"
                //cinterface
                "vulkan_renderer/thirdparty/glslang/glslang/CInterface/glslang_c_interface.cpp",

                //Codegen
                "vulkan_renderer/thirdparty/glslang/glslang/GenericCodeGen/Link.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/GenericCodeGen/CodeGen.cpp",

                //Preprocessor
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp",

                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/limits.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/linkValidate.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/parseConst.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/ParseContextBase.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/ParseHelper.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/PoolAlloc.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/reflection.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/RemoveTree.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/Scan.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/ShaderLang.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/SymbolTable.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/Versions.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/Intermediate.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/Constant.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/attribute.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/glslang_tab.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/InfoSink.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/Initialize.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/intermOut.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/IntermTraverse.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/propagateNoContraction.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/MachineIndependent/iomapper.cpp",

                //OsDependent
                switch (target.result.os.tag) {
                    .linux => "vulkan_renderer/thirdparty/glslang/glslang/OSDependent/Unix/ossource.cpp",
                    .macos => "vulkan_renderer/thirdparty/glslang/glslang/OSDependent/Unix/ossource.cpp",
                    .windows => "vulkan_renderer/thirdparty/glslang/glslang/OSDependent/Windows/ossource.cpp",
                    else => return error.UnsupportedOs,
                },

                "vulkan_renderer/thirdparty/glslang/glslang/ResourceLimits/resource_limits_c.cpp",
                "vulkan_renderer/thirdparty/glslang/glslang/ResourceLimits/ResourceLimits.cpp",

                //SPIRV backend
                "vulkan_renderer/thirdparty/glslang/SPIRV/CInterface/spirv_c_interface.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/GlslangToSpv.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/SpvPostProcess.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/SPVRemapper.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/SpvTools.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/SpvBuilder.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/Logger.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/InReadableOrder.cpp",
                "vulkan_renderer/thirdparty/glslang/SPIRV/doc.cpp",
            },
            .flags = &[_][]const u8{},
        });
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "vulkan_renderer/thirdparty/glslang" });
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "vulkan_renderer/thirdparty/glslang/glslang" });
        vulkan_plugin.addIncludePath(.{ .cwd_relative = "vulkan_renderer/thirdparty/glslang/SPIRV" });
        vulkan_plugin.linkLibrary(glslang_lib);
        vulkan_plugin.root_module.addRPathSpecial("$ORIGIN/Engine");
        vulkan_plugin.root_module.addRPathSpecial("$ORIGIN/../lib");
        vulkan_plugin.root_module.addRPathSpecial("$ORIGIN");
        vulkan_plugin.linkLibrary(libengine);
        //END ************ VULKAN RENDERER PLUGIN ************//

        b.installArtifact(glslang_lib); //use this when the glslang_lib is compiled as a shared library
        b.installArtifact(vulkan_plugin); //use this when the vulkan_plugin is compiled as a shared library

        exe.root_module.addRPathSpecial("$ORIGIN/Engine");
        exe.root_module.addRPathSpecial("$ORIGIN/../lib");
        exe.root_module.addRPathSpecial("$ORIGIN");
        exe.root_module.addRPathSpecial(".");
        exe.linkLibrary(libengine);
        b.installArtifact(libengine); //use this when the engine is compiled as a shared library
        b.installArtifact(exe);
        b.installArtifact(webgpu_plugin); //use this when the webgpu_plugin is compiled as a shared library
        b.installArtifact(testbed_lib); //use this when the game is compiled as a shared library
    }

    if (is_web) {
        const emsdk_binary_name = if (target.result.os.tag == .windows) "/emsdk.bat" else "/emsdk";
        const emsdk_dir = "./deps/emsdk";
        const emsdk_sysroot = emsdk_dir ++ "/upstream/emscripten/cache/sysroot";
        const emsdk_emdawnwebgpu_pkg = emsdk_dir ++ "/upstream/emscripten/cache/ports/emdawnwebgpu/emdawnwebgpu_pkg";
        const install_marker = emsdk_dir ++ "/.emsdk_installed";
        const emsdk_path = b.pathJoin(&.{ emsdk_dir, emsdk_binary_name });

        const emcc_binary_name = if (target.result.os.tag == .windows) "/upstream/emscripten/emcc.bat" else "/upstream/emscripten/emcc";
        const emcc_path = b.pathJoin(&.{ emsdk_dir, emcc_binary_name });
        std.debug.print("Using emcc at: {s}\n", .{emcc_path});

        // Check if emsdk is already installed
        if (!fileExists(install_marker)) {
            std.debug.print("Installing emsdk latest...\n", .{});
            const install_args = [_][]const u8{ emsdk_path, "install", "latest" };
            if (!runCommand(install_args)) {
                std.debug.print("Failed to activate emsdk\n", .{});
                @panic("emsdk activate failed");
            }

            const activate_args = [_][]const u8{ emsdk_path, "activate", "latest" };
            if (!runCommand(activate_args)) {
                std.debug.print("Failed to activate emsdk\n", .{});
                @panic("emsdk activate failed");
            }

            // Create marker file to avoid re-installing next time
            createEmptyFile(install_marker) catch @panic("Failed to create marker file");
        } else {
            std.debug.print("emsdk already installed, skipping install.\n", .{});
        }

        // get headers like webgpu headers
        libengine.addSystemIncludePath(.{ .cwd_relative = b.pathJoin(&.{ emsdk_sysroot, "include" }) });
        const webgpu_include = b.pathJoin(&.{ emsdk_emdawnwebgpu_pkg, "webgpu", "include" });
        libengine.addSystemIncludePath(.{ .cwd_relative = webgpu_include });

        // --- emcc warm step ---
        var warmup_args: [][]const u8 = undefined;

        if (target.result.os.tag == .windows) {
            var arr: [3][]const u8 = .{
                "cmd",
                "/C",
                "echo int main(){} ^| " ++ emsdk_dir ++ "/upstream/emscripten/emcc.bat" ++ " -x c - --use-port=emdawnwebgpu -c -o warm.wasm",
            };
            warmup_args = arr[0..];
        } else {
            var arr: [3][]const u8 = .{
                "sh",
                "-c",
                "echo \"int main(){}\" | " ++ emsdk_dir ++ "/upstream/emscripten/emcc" ++ " -x c - --use-port=emdawnwebgpu -c -o warm.wasm",
            };
            warmup_args = arr[0..];
        }
        const emcc_warmup = b.addSystemCommand(warmup_args);

        // --- Delete warm output step (depends on warm‑up) ---
        var delete_args: [][]const u8 = undefined;
        if (target.result.os.tag == .windows) {
            var arr: [4][]const u8 = .{
                "cmd",
                "/C",
                "del /F warm.wasm",
                "",
            };
            delete_args = arr[0..3]; // drop the unused last slot
        } else {
            var arr: [4][]const u8 = .{
                "sh",
                "-c",
                "rm -f warm.wasm",
                "",
            };
            delete_args = arr[0..3];
        }
        const delete_step = b.addSystemCommand(delete_args);
        delete_step.step.dependOn(&emcc_warmup.step);

        std.debug.print("Engine: emcc linking..\n", .{});
        const emcc = b.addSystemCommand(&.{
            emcc_path,
            "--use-port=emdawnwebgpu",
            "--preload-file",
            "./assets",
            "-o",
            "zig-out/web/app.js",
            "-s FORCE_FILESYSTEM=1",
            "-s",
            "WASM=1",
            "-s",
            "MODULARIZE=1",
            "-s",
            "EXPORT_NAME=Module",
            "-s",
            "EXPORTED_FUNCTIONS=['_start_web', '_em_loop', '_render_loop']",
            "-s",
            "EXPORTED_RUNTIME_METHODS=['cwrap', 'FS']",
            "-s",
            "ALLOW_MEMORY_GROWTH=1",
            "-s",
            "INITIAL_MEMORY=1536MB", // 1.5GB initial memory
            "-s",
            "MAXIMUM_MEMORY=4GB", // 4GB max memory
            "-s",
            "USE_PTHREADS=1",
            //"-s",
            //"PROXY_TO_PTHREAD=1",
            "-s",
            "STACK_SIZE=10MB", // 10MB stack
            "--no-entry", // Don't require a main function
            "-s",
            "ENVIRONMENT=web,worker", // Target web environment
            "-s",
            "PTHREAD_POOL_SIZE=16",
            "-s",
            "ASSERTIONS=1", // Enable assertions for debugging
            //"-sSAFE_HEAP=1", // Additional runtime checks (needed when hunting a segfault)
            //"-sEXCEPTION_DEBUG=1", // Debug exceptions (needed when hunting a throw)
            "-g", // Generate debug info
            "--source-map", // map debug info to C functions
            //"-v", //Verbose log
        });

        emcc.setEnvironmentVariable("EMSDK", emsdk_dir);
        emcc.setEnvironmentVariable("EMSCRIPTEN_SYSROOT", emsdk_sysroot);

        // Add the engine and testbed as inputs
        emcc.addFileArg(libengine.getEmittedBin());

        //emcc.step.dependOn(&emcc_config.step);
        // Make sure the engine and testbed are built first
        libengine.step.dependOn(&emcc_warmup.step);
        libengine.step.dependOn(&delete_step.step);
        emcc.step.dependOn(&libengine.step);

        // Add the emcc step to the install step
        b.getInstallStep().dependOn(&emcc.step);

        // Create web directory if it doesn't exist
        const mkdir_web = b.addSystemCommand(&.{ "mkdir", "-p", "zig-out/web" });
        emcc.step.dependOn(&mkdir_web.step);

        // Copy HTML template
        const copy_html = b.addSystemCommand(&.{ "cp", "web/index.html", "zig-out/web/index.html" });
        copy_html.step.dependOn(&mkdir_web.step);
        b.getInstallStep().dependOn(&copy_html.step);
    }

    if (target.result.abi == .android) {
        const apk: *android.Apk = android_apk orelse @panic("Android APK should be initialized");
        apk.addArtifact(libengine);
        apk.addArtifact(exe);
    }
    if (android_apk) |apk_file| {
        apk_file.installApk();
    }
    if (!is_web) {
        b.installDirectory(.{
            .source_dir = b.path("assets"),
            .install_dir = .prefix,
            .install_subdir = "bin/assets",
        });

        const run_cmd = b.addRunArtifact(exe);
        //run_cmd.cwd = "zig-out/bin"; // (relative to project root)
        run_cmd.cwd = .{ .cwd_relative = b.exe_dir };
        run_cmd.step.dependOn(b.getInstallStep());

        if (b.args) |args| {
            run_cmd.addArgs(args);
        }

        const run_step = b.step("run", "Run the app");
        run_step.dependOn(&run_cmd.step);
    } else {
        //b.installDirectory(.{
        //    .source_dir = b.path("assets"),
        //    .install_dir = .prefix,
        //    .install_subdir = "web/assets",
        //});
        const emrun_binary_name = if (target.result.os.tag == .windows) "emrun.bat" else "emrun"; // adjust path accordingly
        const emrun_path = b.pathJoin(&.{ "deps/emsdk/upstream/emscripten/", emrun_binary_name });
        const run_cmd = b.addSystemCommand(&.{
            emrun_path,
            //"--no_browser",
            //"--port",
            //"8080",
            "zig-out/web",
        });
        run_cmd.step.dependOn(b.getInstallStep());
        const run_step = b.step("run", "Run the web app");
        run_step.dependOn(&run_cmd.step);
    }

    //const unit_tests = b.addExecutable(.{
    //    .name = "test",
    //    .target = target,
    //    .optimize = optimize,
    //});
    //
    //unit_tests.addIncludePath(.{ .cwd_relative = "engine/src" });
    //unit_tests.addIncludePath(.{ .cwd_relative = "test/src" });
    //// Search for all C/C++ files in `src` and add them
    //{
    //    var dir = try std.fs.cwd().openDir("test/src", .{ .iterate = true });
    //
    //    var walker = try dir.walk(b.allocator);
    //    defer walker.deinit();
    //
    //    const allowed_exts = [_][]const u8{".c"};
    //    while (try walker.next()) |entry| {
    //        const ext = std.fs.path.extension(entry.basename);
    //        const include_file = for (allowed_exts) |e| {
    //            if (std.mem.eql(u8, ext, e))
    //                break true;
    //        } else false;
    //        if (include_file) {
    //            std.debug.print("test: Found C file to compile: '{s}'. path: '{s}'\n", .{ entry.basename, entry.path });
    //            unit_tests.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "test/src", entry.path })), .flags = &flags });
    //        }
    //    }
    //}
    //unit_tests.linkLibrary(libengine); //just compile specific files
    //unit_tests.linkLibC();
    //unit_tests.addCSourceFile(.{ .file = b.path("engine/src/core/logger.c"), .flags = &flags });
    //unit_tests.addCSourceFile(.{ .file = b.path("engine/src/core/ystring.c"), .flags = &flags });
    //unit_tests.addCSourceFile(.{ .file = b.path("engine/src/core/ymemory.c"), .flags = &flags });
    //unit_tests.addCSourceFile(.{ .file = b.path("engine/src/math/ymath.c"), .flags = &flags });
    //unit_tests.addCSourceFile(.{ .file = b.path("engine/src/data_structures/darray.c"), .flags = &flags });
    //unit_tests.addCSourceFile(.{ .file = b.path("engine/src/memory/linear_allocator.c"), .flags = &flags });

    //const run_unit_tests = b.addRunArtifact(unit_tests)//;

    //const test_step = b.step("test", "Run unit tests");
    //test_step.dependOn(&run_unit_tests.step);

    // Add Android target options
    //    const target_android = b.standardTargetOptions(.{
    //        .default_target = .{
    //            .cpu_arch = .aarch64,
    //            .os_tag = .android,
    //            .abi = .android,
    //        },
    //    });
    //
    //    // Modify the Android library section to use C files instead:
    //    const android_lib = b.addSharedLibrary(.{
    //        .name = "game_engine",
    //        .target = target_android,
    //        .optimize = optimize,
    //    });
    //
    //    // Add the C source files
    //    android_lib.addCSourceFile(.{
    //        .file = .{ .path = "engine/src/platform/android/platform_android_main.c" },
    //        .flags = &.{"-D_GNU_SOURCE"},
    //    });
    //
    //    android_lib.addCSourceFile(.{
    //        .file = .{ .path = "engine/src/platform/android/android_native_app_glue.c" },
    //        .flags = &.{"-D_GNU_SOURCE"},
    //    });
    //
    //    // Add include directories
    //    android_lib.addIncludePath(.{ .path = "src/platform/android" });
    //
    //    // Link against Android libraries
    //    android_lib.linkSystemLibrary("android");
    //    android_lib.linkSystemLibrary("EGL");
    //    android_lib.linkSystemLibrary("GLESv3");
    //    android_lib.linkSystemLibrary("log");
    //
    //    const android_lib_step = b.addInstallArtifact(android_lib);
    //    b.getInstallStep().dependOn(&android_lib_step.step);
}

fn fileExists(path: []const u8) bool {
    const file_result = std.fs.cwd().openFile(path, .{}) catch return false;
    defer file_result.close();
    return true;
}

fn createEmptyFile(path: []const u8) !void {
    const file = try std.fs.cwd().createFile(path, .{});
    file.close();
}

fn runCommand(argv: [3][]const u8) bool {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    var arena = std.heap.ArenaAllocator.init(gpa.allocator());
    defer arena.deinit();

    var child = std.process.Child.init(&argv, arena.allocator());
    //child.cwd = cwd;
    child.stdin_behavior = .Ignore;

    child.spawn() catch |err| {
        std.debug.print("Failed to spawn child process: {}\n", .{err});
        return false;
    };

    std.debug.print("Child process spawned successfully.\n", .{});

    const term = child.wait() catch |err| {
        std.debug.print("Error while waiting for child process: {}\n", .{err});
        return false;
    };

    return term == .Exited and term.Exited == 0;
}
