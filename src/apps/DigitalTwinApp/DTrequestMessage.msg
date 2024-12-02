//
//                  DT Simulator
//
// Authors: Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

class DTrequestMessage extends inet::FieldsChunk {
    string reqType;
    unsigned int nframes;
    unsigned int IDframe;
    simtime_t arrivalTime;
    simtime_t payloadTimestamp;
    unsigned int payloadSize;
}