
/*
 * Copyright (C) 2015  Johan van der Vleuten
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <SPI.h>
#include <nrf24l01p.h>

NRF24L01p nRF24L01p(9, 10);

uint8_t address1[3];
uint8_t address2[3];

void setup() {
  Serial.begin(115200);

  nRF24L01p.init();

  address1[0] = 0xE7;
  address1[1] = 0xE7;
  address1[2] = 0xE7;

  address2[0] = 0xC2;
  address2[1] = 0xC2;
  address2[2] = 0xC2;

  // Set smallest address width for minimum overhead
  uint8_t address_width = 0b01;
  nRF24L01p.set_address_width(&address_width);

  nRF24L01p.set_enable_crc(1);
  nRF24L01p.set_crco_encoding_scheme(1);

  nRF24L01p.enable_auto_ack(1);

  nRF24L01p.setup_retries(15, 15);

  // 250 kbps, full output
  nRF24L01p.set_rf_setup(0, 1, 0, 0, 0b11);
}

void loop() {
  Serial.println(F("Use 0 to receive packets and use 1 to send packets: "));

  while (Serial.available() == 0)  {  }

  char choice = Serial.read();

  if (choice == '1') {
    nRF24L01p.set_tx_addr(address2, 3);
    nRF24L01p.set_rx_addr_p0(address2, 3);
    nRF24L01p.set_rx_addr_p1(address1, 3);

    unsigned long data = rand();

    unsigned long packets_failed = 0;
    unsigned long packets_sent = 0;

    unsigned long start = millis();
    unsigned long now = start;

    // Put transceiver in standby2 mode for fast transmitting
    nRF24L01p.set_standby2();

    while (1) {

      data++;

      // Wait until there is space in TX FIFO
      while (nRF24L01p.tx_fifo_full()) {
        // Check if package has failed sending
        if (nRF24L01p.tx_max_rt()) {
          // Reset interrupt to retry package
          nRF24L01p.reset_tx_interrupts();
          packets_failed++;
          Serial.print(F("Failed sending packet "));
          Serial.print(data);
          Serial.println(F(", retrying..."));
        }
      }

      // We are in standby2 mode, so transceiver will transmit after
      // writing a payload in the TX FIFO.
      nRF24L01p.write_tx_payload(&data, sizeof ( unsigned long));

      packets_sent++;

      now = millis();
      unsigned long elapsed_milliseconds = now - start;

      if (elapsed_milliseconds > 1000) {
        float bytes_received = packets_sent * 32;
        float rate = bytes_received / elapsed_milliseconds;

        Serial.print(packets_sent);
        Serial.print(F(" packets sent @ "));
        Serial.print(rate);
        Serial.println(F("  KB/s"));

        start = now;
        packets_sent = 0;
      }
    }
  } else {

    nRF24L01p.set_tx_addr(address1, 3);
    nRF24L01p.set_rx_addr_p0(address1, 3);
    nRF24L01p.set_rx_addr_p1(address2, 3);

    // Listen for incoming packets
    nRF24L01p.set_prim_rx();

    unsigned long buf;

    unsigned long received_packets = 0;

    unsigned long start = millis();
    unsigned long now = start;

    while (1) {
      now = millis();
      unsigned long elapsed_milliseconds = now - start;

      // Check for payloads in the RX FIFO. This is faster then
      // first checking rx_data_ready().
      if (nRF24L01p.payloads_available()) {

        nRF24L01p.read_rx_payload(&buf, sizeof (unsigned long));

        buf++;

        received_packets++;
      }

      if (elapsed_milliseconds > 1000) {
        float bytes_received = received_packets * 32;
        float rate = bytes_received / elapsed_milliseconds;

        Serial.print(received_packets);
        Serial.print(F(" packets received @ "));
        Serial.print(rate);
        Serial.println(F("  KB / s"));

        start = now;
        received_packets = 0;
      }
    }
  }
}
