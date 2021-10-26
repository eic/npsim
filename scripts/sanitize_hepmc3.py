#!/usr/bin/env python3

import sys
import argparse

class InvalidHepmc3Error(Exception):
    def __init__(this, *msg):
        print('InvalidHepmc3Error:', *msg, file=sys.stderr)
        
class EventHeader:
    def __init__(this, line):
        if not line[0] == 'E':
            raise InvalidHepmc3Error('Expected event record, but found:', line)
        this.raw = line
        this.vertex = this._get_vertex_if_present(line)
        this.n_vertices = int(line.split(' ')[2])
        this.n_particles = int(line.split(' ')[3])
        this.vert_cnt = 0
        this.part_cnt = 0
    def get_record(this):
        if not this.vert_cnt == this.n_vertices:
            raise InvalidHepmc3Error('Not all vertices found for event:', event.raw)
        if not this.part_cnt == this.n_particles:
            raise InvalidHepmc3Error('Not all particles found for event:', event.raw)
        if this.vertex and not '@' in this.raw:
            return '{} @{}'.format(this.raw[:-1], this.vertex)
        return this.raw
    def process_vertex(this, line):
        this.vert_cnt += 1
        if this.vert_cnt > this.n_vertices:
            raise InvalidHepmc3Error('Too many vertices for event:', event.raw)
        if not this.vertex:
            if '@' in line:
                this.vertex = this._get_vertex_if_present(line)
    def process_particle(this, line):
        this.part_cnt += 1
        if this.part_cnt > this.n_particles:
            raise InvalidHepmc3Error('Too many particles for event:', event.raw)
    def _get_vertex_if_present(this, line):
        if not '@' in line:
            return None
        return line.split('@')[1]

def flush_buffer(header, buffer):
    if header:
        sys.stdout.write(header.get_record())
    for line in buffer:
        sys.stdout.write(line)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            description='Read HepMC3 input from stdin and sanitize the '
                        'output (e.g. add vertex info to the event header '
                        'if the event has a displaced starting vertex). '
                        'Output is written to stdout.')
    args = parser.parse_args()

    header = None
    buffer = []
    have_first_line = False
    have_second_line = False
    end_reached = False
    ## read line-by-line, fill the event buffer, and then
    ## write the sanitized output to stdout
    for line in sys.stdin:
        if end_reached or len(line) == 0:
            continue
        if not have_first_line:
            if not 'HepMC::Version' in line:
                raise InvalidHepmc3Error('Not a valid HepMC3 file header:', line)
            have_first_line = True
            buffer.append(line)
        elif not have_second_line:
            if not 'HepMC::Asciiv3-START_EVENT_LISTING\n' == line:
                raise InvalidHepmc3Error('Not a valid HepMC3 file header:', line)
            have_second_line = True
            buffer.append(line)
        elif 'HepMC::Asciiv3-END_EVENT_LISTING\n' == line:
            end_reached = True
            buffer.append(line)
        elif line[0] == 'E':
            flush_buffer(header, buffer)
            header = EventHeader(line)
            buffer = []
        else:
            if header is None:
                raise InvalidHepmc3Error('Encountered field before the Event header:', line)
            if line[0] == 'V':
                header.process_vertex(line)
            elif line[0] == 'P':
                header.process_particle(line)
            elif line[0] not in ['W', 'A', 'U']:
                raise InvalidHepmc3Error('Encountered unknown field:', line)
            buffer.append(line)
    # final buffer flush at the end
    flush_buffer(header, buffer)
