#include "drivers/vga.hpp"
#include "dma.hpp"

namespace Kernel::DMA {

    enum Registers : u8 {
        START_ADDR_REG_0 = 0x00,
        START_ADDR_REG_1 = 0x02,
        START_ADDR_REG_2 = 0x04,
        START_ADDR_REG_3 = 0x06,
        START_ADDR_REG_4 = 0xC0,
        START_ADDR_REG_5 = 0xC4,
        START_ADDR_REG_6 = 0xC8,
        START_ADDR_REG_7 = 0xCC,

        PAGE_ADDR_REG_0 = 0x87,
        PAGE_ADDR_REG_1 = 0x83,
        PAGE_ADDR_REG_2 = 0x81,
        PAGE_ADDR_REG_3 = 0x82,
        PAGE_ADDR_REG_4 = 0x8F,
        PAGE_ADDR_REG_5 = 0x8B,
        PAGE_ADDR_REG_6 = 0x89,
        PAGE_ADDR_REG_7 = 0x8A,

        COUNT_REG_0 = 0x01,
        COUNT_REG_1 = 0x03,
        COUNT_REG_2 = 0x05,
        COUNT_REG_3 = 0x07,
        COUNT_REG_4 = 0xC2,
        COUNT_REG_5 = 0xC6,
        COUNT_REG_6 = 0xCA,
        COUNT_REG_7 = 0xCE,

        SINGLE_CHANNEL_MASK_REG_0_3 = 0x0A,
        SINGLE_CHANNEL_MASK_REG_4_7 = 0xD4,

        FLIP_FLOP_RESET_REG_0_3 = 0x0C,
        FLIP_FLOP_RESET_REG_4_7 = 0xD8,

        MODE_REG_0_3 = 0x0B,
        MODE_REG_4_7 = 0xD6
    };

    SZNN::ErrorOr<u8> get_start_address_port(u8 t_channel) {
        ASSERT(t_channel < 8, -1); // TODO: error code

        const u8 REGISTERS[8] = {
            START_ADDR_REG_0,
            START_ADDR_REG_1,
            START_ADDR_REG_2,
            START_ADDR_REG_3,
            START_ADDR_REG_4,
            START_ADDR_REG_5,
            START_ADDR_REG_6,
            START_ADDR_REG_7
        };

        return REGISTERS[t_channel];
    }

    SZNN::ErrorOr<u8> get_page_address_port(u8 t_channel) {
        ASSERT(t_channel < 8, -1); // TODO: error code

        const u8 REGISTERS[8] = {
            PAGE_ADDR_REG_0,
            PAGE_ADDR_REG_1,
            PAGE_ADDR_REG_2,
            PAGE_ADDR_REG_3,
            PAGE_ADDR_REG_4,
            PAGE_ADDR_REG_5,
            PAGE_ADDR_REG_6,
            PAGE_ADDR_REG_7
        };

        return REGISTERS[t_channel];
    }

    SZNN::ErrorOr<u8> get_count_port(u8 t_channel) {
        ASSERT(t_channel < 8, -1); // TODO: error code

        const u8 REGISTERS[8] = {
            COUNT_REG_0,
            COUNT_REG_1,
            COUNT_REG_2,
            COUNT_REG_3,
            COUNT_REG_4,
            COUNT_REG_5,
            COUNT_REG_6,
            COUNT_REG_7
        };

        return REGISTERS[t_channel];
    }

    SZNN::ErrorOr<u8> get_single_channel_mask(u8 t_channel) {
        ASSERT(t_channel < 8, -1); // TODO: error code
        return (t_channel < 4) ? (SINGLE_CHANNEL_MASK_REG_0_3) : (SINGLE_CHANNEL_MASK_REG_4_7);
    }

    SZNN::ErrorOr<u8> get_flip_flop_reset(u8 t_channel) {
        ASSERT(t_channel < 8, -1); // TODO: error code
        return (t_channel < 4) ? (FLIP_FLOP_RESET_REG_0_3) : (FLIP_FLOP_RESET_REG_4_7);
    }

    SZNN::ErrorOr<u8> get_mode(u8 t_channel) {
        ASSERT(t_channel < 8, -1); // TODO
        return (t_channel < 4) ? (MODE_REG_0_3) : (MODE_REG_4_7);
    }

    SZNN::ErrorOr<void> initialize_channel(u8 t_channel, void* t_bufferAddress, u16 t_count) {
        const u8 startAddressPort = TRY(get_start_address_port(t_channel));
        const u8 pageAddressPort = TRY(get_page_address_port(t_channel));
        const u8 countPort = TRY(get_count_port(t_channel));
        const u8 channelMaskPort = TRY(get_single_channel_mask(t_channel));
        const u8 flipFlopResetPort = TRY(get_flip_flop_reset(t_channel));

        port_write_byte(channelMaskPort, (t_channel & 0x03) | 0x04); // mask the DMA channel to be initialized

        const u32 bufferAddress = u32(t_bufferAddress);
        port_write_byte(flipFlopResetPort, 0xFF); // reset flip flop to low byte
        port_write_byte(startAddressPort, (bufferAddress >> 0) & 0xFF); // write low byte of start address
        port_write_byte(startAddressPort, (bufferAddress >> 8) & 0xFF); // write mid byte of start address
        port_write_byte(pageAddressPort, (bufferAddress >> 16) & 0xFF); // write high byte of start address

        port_write_byte(flipFlopResetPort, 0xFF); // reset flip flop
        port_write_byte(countPort, (t_count >> 0) & 0xFF); // write low byte of count
        port_write_byte(countPort, (t_count >> 8) & 0xFF); // write high byte of count

        port_write_byte(channelMaskPort, t_channel & 0x03); // unmask the DMA channel to be initialized

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> set_mode(u8 t_channel, u8 t_transferType, bool t_autoInit, bool t_down, u8 t_mode) {
        const u8 channelMaskPort = TRY(get_single_channel_mask(t_channel));
        const u8 modePort = TRY(get_mode(t_channel));
        const u8 parameter =
            ((t_mode         & 0x03) << 6) |
            ((!!t_down       & 0x01) << 5) |
            ((!!t_autoInit   & 0x01) << 4) |
            ((t_transferType & 0x03) << 2) |
            ((t_channel      & 0x03) << 0);

        port_write_byte(channelMaskPort, (t_channel & 0x03) | 0x04); // mask the DMA channel to be initialized

        port_write_byte(modePort, parameter);

        port_write_byte(channelMaskPort, t_channel & 0x03); // unmask the DMA channel to be initialized

        return SZNN::ErrorOr<void>();
    }

}
