const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const flags = [_][]const u8{
        "",
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
                std.debug.print("Engine: Found C file to compile: '{s}'. path: '{s}'\n", .{ entry.basename, entry.path });
                libengine.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "engine/src", entry.path })), .flags = &flags });
            }
        }
    }

    if (target.result.os.tag == .linux) {
        if (b.lazyDependency("x11_headers", .{
            .target = target,
            .optimize = optimize,
        })) |dep| {
            libengine.linkLibrary(dep.artifact("x11-headers"));
        }
        //        if (b.lazyDependency("wayland_headers", .{
        //            .target = target,
        //            .optimize = optimize,
        //        })) |dep| {
        //            libengine.linkLibrary(dep.artifact("wayland-headers"));
        //        }
        libengine.linkSystemLibrary("xcb");
        libengine.linkSystemLibrary("X11");
        libengine.linkSystemLibrary("X11-xcb");
        libengine.linkSystemLibrary("xkbcommon");
        libengine.addLibraryPath(.{ .cwd_relative = "/usr/X11R6/lib" });
    }

    if (b.lazyDependency("vulkan_headers", .{
        .target = target,
        .optimize = optimize,
    })) |dep| {
        libengine.linkLibrary(dep.artifact("vulkan-headers"));
    }

    if (b.lazyDependency("wgpu", .{
        .target = target,
        .optimize = optimize,
    })) |dep| {
        libengine.addIncludePath(dep.path("include"));

        //In The Future I Should replace this with compiling WGPU from source to support more platforms & statically link WGPU
        var lib_file = dep.path("bin");
        if (target.result.os.tag == .linux) {
            if (target.result.cpu.arch == .aarch64) {
                lib_file = dep.path("bin/linux-aarch64");
            } else if (target.result.cpu.arch == .x86_64) {
                lib_file = dep.path("bin/linux-x86_64");
            }
        } else if (target.result.os.tag == .windows) {
            if (target.result.cpu.arch == .aarch64) {
                lib_file = dep.path("bin/windows-i686");
            } else if (target.result.cpu.arch == .x86_64) {
                lib_file = dep.path("bin/windows-x86_64");
            }
        } else if (target.result.os.tag == .macos) {
            if (target.result.cpu.arch == .aarch64) {
                lib_file = dep.path("bin/macos-aarch64");
            } else if (target.result.cpu.arch == .x86_64) {
                lib_file = dep.path("bin/macos-x86_64");
            }
        }

        b.installDirectory(.{
            .source_dir = lib_file,
            .install_dir = .prefix,
            .install_subdir = "bin/",
        });
        libengine.root_module.addRPathSpecial("$ORIGIN");
        libengine.root_module.addRPathSpecial(".");
        libengine.addLibraryPath(lib_file);
        if (target.result.os.tag == .windows) {
            libengine.linkSystemLibrary2("wgpu_native.dll", .{
                .preferred_link_mode = .dynamic,
            });
        } else {
            libengine.linkSystemLibrary2("wgpu_native", .{
                .preferred_link_mode = .dynamic,
            });
        }
    }

    libengine.linkLibC();

    const exe = b.addExecutable(.{
        .name = "testbed",
        .target = target,
        .optimize = optimize,
    });

    exe.addIncludePath(.{ .cwd_relative = "engine/src" });
    exe.addIncludePath(.{ .cwd_relative = "testbed/src" });
    // Search for all C/C++ files in `src` and add them
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
                std.debug.print("Testbed: Found C file to compile: '{s}'. path: '{s}'\n", .{ entry.basename, entry.path });
                exe.addCSourceFile(.{ .file = b.path(b.pathJoin(&.{ "testbed/src", entry.path })), .flags = &flags });
            }
        }
    }

    exe.root_module.addRPathSpecial("$ORIGIN");
    exe.root_module.addRPathSpecial(".");
    exe.linkLibrary(libengine);

    b.installArtifact(libengine); //use this when the engine is compiled as a shared library

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    const unit_tests = b.addTest(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    const run_unit_tests = b.addRunArtifact(unit_tests);

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_unit_tests.step);
}
