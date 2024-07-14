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

    const libengine = b.addStaticLibrary(.{ //addStaticLibrary/addSharedLibrary
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
                libengine.addCSourceFile(.{ .file = .{ .path = b.pathJoin(&.{ "engine/src", entry.path }) }, .flags = &flags });
            }
        }
    }

    if (target.result.os.tag == .linux) {
        libengine.linkSystemLibrary("xcb");
        libengine.linkSystemLibrary("X11");
        libengine.linkSystemLibrary("X11-xcb");
        libengine.linkSystemLibrary("xkbcommon");
        libengine.linkSystemLibrary("vulkan");
        libengine.addLibraryPath(.{ .cwd_relative = "/usr/X11R6/lib" });
    } else if (target.result.os.tag == .windows) {
        libengine.linkSystemLibrary("vulkan-1");
    }

    const env_map = try std.process.getEnvMap(b.allocator);
    if (env_map.get("VULKAN_SDK")) |path| {
        libengine.addLibraryPath(.{ .cwd_relative = std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) catch @panic("OOM") });
        libengine.addIncludePath(.{ .cwd_relative = std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) catch @panic("OOM") });
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
                exe.addCSourceFile(.{ .file = .{ .path = b.pathJoin(&.{ "testbed/src", entry.path }) }, .flags = &flags });
            }
        }
    }

    exe.linkLibrary(libengine);

    //b.installArtifact(libengine); use this when the engine is compiled as a shared library

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
