const std = @import("std");

pub extern fn engine_main(backend: RendererBackend) i32;

// The main entry point of the application.

// Define renderer options (matches C enum)
pub const RendererBackend = enum(c_int) {
    VULKAN, // 0 (default)
    WEBGPU, // 1
    _,
};
// Web entry point
pub export fn start_web() void {
    //std.debug.print("Zig starting C engine...\n", .{});
    const backend: RendererBackend = .WEBGPU;
    _ = engine_main(backend);
}
pub fn main() !void {
    const allocator = std.heap.page_allocator;
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    var backend: RendererBackend = .VULKAN; // Default

    // Parse --renderer=<value>
    for (args) |arg| {
        if (std.mem.startsWith(u8, arg, "--renderer=")) {
            const value = arg["--renderer=".len..];
            backend = std.meta.stringToEnum(RendererBackend, value) orelse {
                std.debug.print("Unknown renderer: '{s}'. Using Vulkan.\n", .{value});
                continue;
            };
            std.debug.print("Renderer set to: {s}\n", .{value});
        }
    }

    // Pass the parsed backend to C
    _ = engine_main(backend);
}
