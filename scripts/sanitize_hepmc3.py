#!/usr/bin/env python3

import sys
import argparse

class InvalidHepmc3Error(Exception):
    def __init__(this, *msg):
        print('InvalidHepmc3Error:', *msg, file=sys.stderr)

def warn(*msg):
    print('WARNING:', *msg, file=sys.stderr)

        
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
        if this.vert_cnt > this.n_vertices:
            warn('Too many vertices for event:', this.raw, "--> skipping event")
            return None
        if not this.part_cnt == this.n_particles:
            warn('Invalid particle count for event:', this.raw, "--> skipping event")
            return None
        if this.vertex and not '@' in this.raw:
            return '{} @{}'.format(this.raw[:-1], this.vertex)
        return this.raw
    def process_vertex(this, line):
        this.vert_cnt += 1
        if this.vert_cnt > this.n_vertices:
            warn('Too many vertices for event:', this.raw)
            return
        if not this.vertex:
            if '@' in line:
                this.vertex = this._get_vertex_if_present(line)
    def process_particle(this, line):
        this.part_cnt += 1
        if this.part_cnt > this.n_particles:
            warn('Too many particles for event:', this.raw)
            return
    def _get_vertex_if_present(this, line):
        if not '@' in line:
            return None
        return line.split('@')[1]

def flush_buffer(header, buffer):
    if header:
        event_record = header.get_record()
        if not event_record:
            warn('Skipped invalid event', header.raw)
            return
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
        if len(line) == 0:
            continue
        if end_reached:
            warn("Ignoring lines after END_EVENT_LISTING was reached")
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
        elif line[0] in ['A','W','T','N']:
            buffer.append(line)
        elif line[0] == 'E':
            flush_buffer(header, buffer)
            header = EventHeader(line)
            buffer = []
        else:
            if header is None:
                raise InvalidHepmc3Error('Encountered invalid field before the first Event header:', line)
            if line[0] == 'V':
                header.process_vertex(line)
            elif line[0] == 'P':
                header.process_particle(line)
            elif line[0] not in ['W', 'A', 'U']:
                warn('Ignoring unknown field:', line)
                continue
            buffer.append(line)
    if not end_reached:
        warn("File does not end with END_EVENT_LISTING, appending")
        buffer.append('HepMC::Asciiv3-END_EVENT_LISTING\n')
    # final buffer flush at the end
    flush_buffer(header, buffer)
