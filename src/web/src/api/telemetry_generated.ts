import * as flatbuffers from "flatbuffers";

export namespace Telemetry {
  export class HandEquity {
    bb: flatbuffers.ByteBuffer | null = null;
    bb_pos = 0;

    __init(i: number, bb: flatbuffers.ByteBuffer): HandEquity {
      this.bb_pos = i;
      this.bb = bb;
      return this;
    }

    handName(): string | null {
      const offset = this.bb!.__offset(this.bb_pos, 4);
      return offset ? this.bb!.__string(this.bb_pos + offset) : null;
    }

    equity(): number {
      const offset = this.bb!.__offset(this.bb_pos, 6);
      return offset ? this.bb!.readFloat64(this.bb_pos + offset) : 0.0;
    }

    wins(): number {
      const offset = this.bb!.__offset(this.bb_pos, 8);
      return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
    }

    ties(): number {
      const offset = this.bb!.__offset(this.bb_pos, 10);
      return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
    }

    losses(): number {
      const offset = this.bb!.__offset(this.bb_pos, 12);
      return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
    }

    simulations(): number {
      const offset = this.bb!.__offset(this.bb_pos, 14);
      return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
    }
  }

  export class TelemetryPacket {
    bb: flatbuffers.ByteBuffer | null = null;
    bb_pos = 0;

    __init(i: number, bb: flatbuffers.ByteBuffer): TelemetryPacket {
      this.bb_pos = i;
      this.bb = bb;
      return this;
    }

    static getRootAsTelemetryPacket(
      bb: flatbuffers.ByteBuffer,
      obj?: TelemetryPacket
    ): TelemetryPacket {
      // Read root offset from position 0 (first 4 bytes)
      // This is the standard FlatBuffers format after builder.Finish()
      const rootOffset = bb.readInt32(0);

      // Root table position is the offset value itself
      return (obj || new TelemetryPacket()).__init(rootOffset, bb);
    }

    timestampNs(): bigint {
      const offset = this.bb!.__offset(this.bb_pos, 4);
      return offset
        ? this.bb!.readUint64(this.bb_pos + offset)
        : BigInt(0);
    }

    handsProcessed(): bigint {
      const offset = this.bb!.__offset(this.bb_pos, 6);
      return offset
        ? this.bb!.readUint64(this.bb_pos + offset)
        : BigInt(0);
    }

    cpuPercent(): number {
      const offset = this.bb!.__offset(this.bb_pos, 8);
      return offset ? this.bb!.readFloat64(this.bb_pos + offset) : 0.0;
    }

    memoryRssKb(): bigint {
      const offset = this.bb!.__offset(this.bb_pos, 10);
      return offset
        ? this.bb!.readUint64(this.bb_pos + offset)
        : BigInt(0);
    }

    memoryVmsKb(): bigint {
      const offset = this.bb!.__offset(this.bb_pos, 12);
      return offset
        ? this.bb!.readUint64(this.bb_pos + offset)
        : BigInt(0);
    }

    threadCount(): number {
      const offset = this.bb!.__offset(this.bb_pos, 14);
      return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
    }

    cpuCycles(): bigint {
      const offset = this.bb!.__offset(this.bb_pos, 16);
      return offset
        ? this.bb!.readUint64(this.bb_pos + offset)
        : BigInt(0);
    }

    status(): number {
      const offset = this.bb!.__offset(this.bb_pos, 18);
      return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
    }

    equityResults(index: number, obj?: HandEquity): HandEquity | null {
      const offset = this.bb!.__offset(this.bb_pos, 20);
      return offset
        ? (obj || new HandEquity()).__init(
            this.bb!.__indirect(
              this.bb!.__vector(this.bb_pos + offset) + index * 4
            ),
            this.bb!
          )
        : null;
    }

    equityResultsLength(): number {
      const offset = this.bb!.__offset(this.bb_pos, 20);
      return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
    }
  }
}
