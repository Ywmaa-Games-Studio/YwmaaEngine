const std = @import("std");
const builtin = @import("builtin");
const android = @import("android");

pub fn build(b: *std.Build) !void {
    const exe_name: []const u8 = "testbed";
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const android_targets = android.standardTargets(b, target);

    // If building with Android, initialize the tools / build
    const android_apk: ?*android.APK = blk: {
        if (android_targets.len == 0) {
            break :blk null;
        }
        const android_tools = android.Tools.create(b, .{
            .api_level = .android15,
            .build_tools_version = "35.0.0",
            .ndk_version = "28.0.13004108", //"29.0.13113456",//"27.0.12077973",
        });
        const apk = android.APK.create(b, android_tools);

        const key_store_file = android_tools.createKeyStore(android.CreateKey.example());
        apk.setKeyStore(key_store_file);
        apk.setAndroidManifest(b.path("android/AndroidManifest.xml"));
        apk.addResourceDirectory(b.path("android/res"));

        // Add Java files
        apk.addJavaSourceFile(.{ .file = b.path("android/src/NativeInvocationHandler.java") });
        break :blk apk;
    };

    const engine_flags = [_][]const u8{
        "-DYEXPORT",
        "-DDEBUG",
    };
    const testbed_flags = [_][]const u8{
        "-DYIMPORT",
    };
    // Engine Flag -DYEXPORT
    // Testbed Flag -DYIMPORT

    const libengine = b.addSharedLibrary(.{ //addStaticLibrary/addSharedLibrary
        .name = "engine",
        .target = target,
        .optimize = optimize,
    });

    libengine.addIncludePath(.{ .cwd_relative = "engine/src" });
    // Search for all C files in `src` and add them
    {
        var dir = try std.fs.cwd().openDir("engine/src", .{ .iterate = true });

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
                std.debug.print("Engine: Found {s} file to compile: '{s}'. path: '{s}'\n", .{ ext, entry.basename, entry.path });
                libengine.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "engine/src", entry.path })), .flags = &engine_flags });
            }
        }
    }

    //In The Future I Should replace this with compiling WGPU from source to be able to statically link WGPU
    var lib_file = b.path("engine/thirdparty/wgpu/bin");
    if (target.result.os.tag == .linux) {
        if (target.result.cpu.arch == .aarch64) {
            if (target.result.abi == .android) {
                lib_file = b.path("engine/thirdparty/wgpu/bin/linux-aarch64");
            } else {
                lib_file = b.path("engine/thirdparty/wgpu/bin/linux-aarch64");
            }
        } else if (target.result.cpu.arch == .x86_64) {
            if (target.result.abi == .android) {
                lib_file = b.path("engine/thirdparty/wgpu/bin/linux-x86_64");
            } else {
                lib_file = b.path("engine/thirdparty/wgpu/bin/linux-x86_64");
            }
        }
    } else if (target.result.os.tag == .windows) {
        if (target.result.cpu.arch == .aarch64) {
            lib_file = b.path("engine/thirdparty/wgpu/bin/windows-i686-msvc");
        } else if (target.result.cpu.arch == .x86_64) {
            lib_file = b.path("engine/thirdparty/wgpu/bin/windows-x86_64-msvc");
        }
    } else if (target.result.os.tag == .macos) {
        if (target.result.cpu.arch == .aarch64) {
            lib_file = b.path("engine/thirdparty/wgpu/bin/macos-aarch64");
        } else if (target.result.cpu.arch == .x86_64) {
            lib_file = b.path("engine/thirdparty/wgpu/bin/macos-x86_64");
        }
    }

    libengine.root_module.addRPathSpecial("$ORIGIN");
    libengine.root_module.addRPathSpecial("$ORIGIN/lib");
    libengine.root_module.addRPathSpecial("$ORIGIN/../lib");
    libengine.root_module.addRPathSpecial(".");
    if (target.result.os.tag == .windows) {
        b.installDirectory(.{
            .source_dir = lib_file,
            .install_dir = .prefix,
            .install_subdir = "bin/",
            .include_extensions = &.{".dll"},
        });
        libengine.addLibraryPath(lib_file);
        libengine.linkSystemLibrary2("wgpu_native.dll", .{
            .use_pkg_config = .no,
            .preferred_link_mode = .dynamic,
        });
    } else {
        b.installDirectory(.{
            .source_dir = lib_file,
            .install_dir = .prefix,
            .install_subdir = "lib/",
            .include_extensions = &.{ ".dylib", ".so" },
        });
        libengine.addLibraryPath(lib_file);
        libengine.linkSystemLibrary2("wgpu_native", .{
            .use_pkg_config = .no,
            .preferred_link_mode = .dynamic,
        });
    }

    if (target.result.os.tag == .linux and target.result.abi != .android) {
        if (b.lazyDependency("x11_headers", .{
            .target = target,
            .optimize = optimize,
        })) |dep| {
            libengine.linkLibrary(dep.artifact("x11-headers"));
        }
        if (b.lazyDependency("wayland_headers", .{
            .target = target,
            .optimize = optimize,
        })) |dep| {
            libengine.linkLibrary(dep.artifact("wayland-headers"));
        }
        libengine.linkSystemLibrary("wayland-client");
        libengine.linkSystemLibrary("wayland-cursor");
    }

    if (b.lazyDependency("vulkan_headers", .{
        .target = target,
        .optimize = optimize,
    })) |dep| {
        libengine.linkLibrary(dep.artifact("vulkan-headers"));
    }

    libengine.linkLibC();

    var exe: *std.Build.Step.Compile = if (target.result.abi.isAndroid()) b.addSharedLibrary(.{
        .name = exe_name,
        .root_source_file = b.path("testbed/src/main.zig"),
        .target = target,
        .optimize = optimize,
    }) else b.addExecutable(.{
        .name = exe_name,
        .root_source_file = b.path("testbed/src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    exe.addIncludePath(.{ .cwd_relative = "engine/src" });
    exe.addIncludePath(.{ .cwd_relative = "testbed/src" });
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
                exe.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "testbed/src", entry.path })), .flags = &testbed_flags });
            }
        }
    }
    exe.root_module.addRPathSpecial("$ORIGIN/Engine");
    exe.root_module.addRPathSpecial("$ORIGIN/../lib");
    exe.root_module.addRPathSpecial("$ORIGIN");
    exe.root_module.addRPathSpecial(".");
    exe.linkLibrary(libengine);

    b.installArtifact(libengine); //use this when the engine is compiled as a shared library

    b.installArtifact(exe);
    if (target.result.abi == .android) {
        const apk: *android.APK = android_apk orelse @panic("Android APK should be initialized");
        apk.addArtifact(libengine);
        apk.addArtifact(exe);
    }
    if (android_apk) |apk_file| {
        apk_file.installApk();
    }

    try addShader(b, exe, "builtin.shader.vert.glsl", "builtin.shader.vert.spv", "-fshader-stage=vert");
    try addShader(b, exe, "builtin.shader.frag.glsl", "builtin.shader.frag.spv", "-fshader-stage=frag");
    b.installDirectory(.{
        .source_dir = b.path("assets"),
        .install_dir = .prefix,
        .install_subdir = "bin/assets",
    });

    const run_cmd = b.addRunArtifact(exe);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

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
    //unit_tests.addCSourceFile(.{ .file = b.path("engine/src/variants/darray.c"), .flags = &flags });
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

fn addShader(b: *std.Build, exe: anytype, in_file: []const u8, out_file: []const u8, additional_arg: []const u8) !void {
    // example:
    // glslc $additional_arg shaders/vert.spv -o shaders/shader.vert
    const dirname = "assets/shaders";
    const full_in = b.pathJoin(&.{ dirname, in_file });
    const full_out = b.pathJoin(&.{ dirname, out_file });
    const run_cmd = b.addSystemCommand(&[_][]const u8{
        "glslc",
        additional_arg,
        full_in,
        "-o",
        full_out,
    });
    exe.step.dependOn(&run_cmd.step);
}
