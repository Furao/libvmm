// Copyright 2023, UNSW
// SPDX-License-Identifier: BSD-2-Clause

const std = @import("std");
const c = @cImport({
    @cInclude("linux.h");
    @cInclude("virq.h");
    @cInclude("guest.h");
    @cInclude("vcpu.h");
    @cInclude("tcb.h");
});
const microkit = @import("libmicrokit.zig");

const GUEST_BOOT_VCPU_ID = 0;
const GUEST_RAM_VADDR: usize = 0x40000000;
const GUEST_DTB_VADDR: usize = 0x4f000000;
const GUEST_INIT_RAM_DISK_VADDR: usize = 0x4d700000;
const GUEST_RAM_SIZE: usize = 0x10000000;

const guest_image_path = "images/";
// Data for the guest's kernel image.
const guest_kernel_image = blk: {
    const arr align(@alignOf(c.linux_image_header)) = @embedFile(guest_image_path ++ "linux").*;
    break :blk &arr;
};
// Data for the device tree to be passed to the kernel.
const guest_dtb_image = @embedFile(guest_image_path ++ "linux.dtb");
// Data for the initial RAM disk to be passed to the kernel.
const guest_initrd_image = @embedFile(guest_image_path ++ "rootfs.cpio.gz");

const LinuxKernelImage = extern struct {
    header: c.linux_image_header,
    bytes: *u8
};

// In Zig the standard library comes with printf-like functionality with the
// ability to provide your own function to ouput the characters. This is
// extremely useful for us! Without changing the standard library or bringing
// in a 3rd party library, in a couple of lines of code we can get formatted
// printing functionality that ends up calling to microkit_dbg_puts which then
// outputs to the platform's serial connection.
//
// In a production system you might want to instead hook up the logger to a
// user-level UART driver, but for the example we only need logging for debug
// mode.
const log = struct {
    const Writer = std.io.Writer(u32, error{}, debug_uart_put_str);
    const debug_uart = Writer { .context = 0 };

    fn debug_uart_put_str(_: u32, str: []const u8) !usize {
        microkit.microkit_dbg_puts(@ptrCast(str));
        return str.len;
    }

    pub fn info(comptime fmt: []const u8, args: anytype) void {
        debug_uart.print("VMM|INFO: " ++ fmt ++ "\n", args) catch {};
    }

    pub fn err(comptime fmt: []const u8, args: anytype) void {
        debug_uart.print("VMM|ERROR: " ++ fmt ++ "\n", args) catch {};
    }
};

const SERIAL_IRQ_CH: microkit.microkit_channel = 1;
const SERIAL_IRQ: i32 = 33;

fn serial_ack(_: usize, _: c_int, _: ?*anyopaque) callconv(.C) void {
    // Nothing else needs to be done other than acking the IRQ.
    microkit.microkit_irq_ack(SERIAL_IRQ_CH);
}

export fn init() callconv(.C) void {
    // Initialise the VMM, the VCPU(s), and start the guest
    log.info("starting \"{s}\"\n", .{ microkit.microkit_name });
    // Place all the binaries in the right locations before starting the guest
    const kernel_pc = c.linux_setup_images(
                GUEST_RAM_VADDR,
                @intFromPtr(guest_kernel_image),
                guest_kernel_image.len,
                @intFromPtr(guest_dtb_image),
                GUEST_DTB_VADDR,
                guest_dtb_image.len,
                @intFromPtr(guest_initrd_image),
                GUEST_INIT_RAM_DISK_VADDR,
                guest_initrd_image.len
            );

    if (kernel_pc == 0) {
        log.err("Failed to initialise guest images\n", .{});
        return;
    }
    // Initialise the virtual GIC driver
    var success = c.virq_controller_init(GUEST_BOOT_VCPU_ID);
    if (!success) {
        log.err("Failed to initialise emulated interrupt controller\n", .{});
        return;
    }
    // @ivanv: Note that remove this line causes the VMM to fault if we
    // actually get the interrupt. This should be avoided by making the VGIC driver more stable.
    success = c.virq_register(GUEST_BOOT_VCPU_ID, SERIAL_IRQ, &serial_ack, null);
    // Just in case there is already an interrupt available to handle, we ack it here.
    microkit.microkit_irq_ack(SERIAL_IRQ_CH);
    // Finally start the guest
    success = c.guest_start(GUEST_BOOT_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    std.debug.assert(success);
}

export fn notified(ch: microkit.microkit_channel) callconv(.C) void {
    switch (ch) {
        SERIAL_IRQ_CH => {
            const success = c.virq_inject(GUEST_BOOT_VCPU_ID, SERIAL_IRQ);
            if (!success) {
                log.err("IRQ {x} dropped on vCPU {x}\n", .{ SERIAL_IRQ, GUEST_BOOT_VCPU_ID });
            }
        },
        else => log.err("Unexpected channel, ch: {x}\n", .{ ch })
    }
}

extern fn fault_handle(id: microkit.microkit_id, msginfo: microkit.microkit_msginfo) callconv(.C) bool;

export fn fault(id: microkit.microkit_id, msginfo: microkit.microkit_msginfo) callconv(.C) void {
    if (fault_handle(id, msginfo)) {
        // Now that we have handled the fault, we reply to it so that the guest can resume execution.
        microkit.microkit_fault_reply(microkit.microkit_msginfo_new(0, 0));
    } else {
        log.err("Failed to handle fault\n", .{});
    }
}
