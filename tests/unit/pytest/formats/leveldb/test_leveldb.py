#!/usr/bin/env python

import pytest

from bitpunch import model
import conftest

#
# Test LevelDB formats
#
# log format:
# https://github.com/google/leveldb/blob/master/doc/log_format.md
#
# One interesting characteristic is that blocks are fixed-sized 32KB,
# except the last block which can be truncated. This forces the browse
# code to skip to the tail block whenever there is less than 32KB to
# read from the file.
#
# Otherwise it's a pretty straightforward format.
#

@pytest.fixture
def spec_log():
    fmt = """
    let FixInt = integer { signed: false; endian: 'little'; };

    let FixInt8 =  byte     <> FixInt;
    let FixInt16 = [2] byte <> FixInt;
    let FixInt32 = [4] byte <> FixInt;

    file {
        head_blocks: [] LogBlock;
        tail_block: LogTailBlock;
    }

    let LogBlock = struct {
           records: [] Record;
           trailer: [] byte;
           @span: 32768;
    };

    let LogTailBlock = struct {
           records: [] Record;
    };

    let Record = struct {
           checksum: FixInt32;
           length:   FixInt16;
           rtype:    FixInt8;
           data:     [length] byte <> string;
           @minspan: 7;
    };
"""

    return model.FormatSpec(fmt)


@pytest.fixture
def data_log_empty():
    return conftest.to_bytes("""
    """)

@pytest.fixture
def data_log_small():
    # Log file generated by the "level" CLI tool after putting two
    # key/value pairs
    return conftest.to_bytes("""
    1b cc 27 c2 21 00 01
    01 00 00 00 00 00 00 00 01 00 00 00 01
    09 '"coolkey"' 09 'coolvalue'
    95 c4 c2 6e 27 00 01 02
    00 00 00 00 00 00 00 01  00 00 00 01
    0c '"coolnewkey"' 0c 'coolnewvalue'
    """)

@pytest.fixture
def data_log_multiblock():
    # create a 43 bytes record
    record = conftest.to_bytes("""
    1b cc 27 c2 24 00 01
    01 00 00 00 00 00 00 00 01 00 00 00 01
    09 '"coolkey!"' 09 'coolvalue!!'
    """)

    #43 * 762 == 32766: add 2 padding bytes before next block begins
    return (record * 762
            + conftest.to_bytes('00 00')
            + record * 3)


def test_leveldb_log_browse(spec_log, data_log_empty, data_log_small):
    inputs = [data_log_empty, data_log_small]
    for data in inputs:
        dtree = model.DataTree(data, spec_log)
        block_count = 0
        for block in dtree.head_blocks:
            block_count += 1
            record_count = 0
            for record in block.records:
                record_count += 1
            assert record_count == len(block.records)
        assert block_count == len(dtree.head_blocks)

        block = dtree.tail_block
        record_count = 0
        for record in block.records:
            record_count += 1
        assert record_count == len(block.records)


def test_leveldb_log_empty(spec_log, data_log_empty):
    dtree = model.DataTree(data_log_empty, spec_log)
    assert model.make_python_object(dtree.head_blocks) == []
    assert model.make_python_object(dtree.tail_block.records) == []
    assert len(dtree.head_blocks) == 0
    assert len(dtree.tail_block.records) == 0

    assert dtree.eval_expr('sizeof(head_blocks)') == 0
    assert dtree.eval_expr('sizeof(tail_block.records)') == 0

    with pytest.raises(IndexError):
        dtree.tail_block.records[0]

    with pytest.raises(ValueError):
        dtree.eval_expr('tail_block.records[0]')


def test_leveldb_log_small(spec_log, data_log_small):
    dtree = model.DataTree(data_log_small, spec_log)
    assert model.make_python_object(dtree.head_blocks) == []
    records = dtree.tail_block.records
    assert len(records) == 2

    assert records[0].checksum == 0xC227CC1B
    assert records[0].length == 33
    assert records[0].rtype == 1
    assert len(records[0].data) == 33
    assert dtree.eval_expr('sizeof(tail_block.records[0])') == 40

    assert records[1].checksum == 0x6EC2C495
    assert records[1].length == 39
    assert records[1].rtype == 1
    assert len(records[1].data) == 39
    assert dtree.eval_expr('sizeof(tail_block.records[1])') == 46

    assert dtree.eval_expr('sizeof(tail_block.records)') == 86
    assert dtree.eval_expr('sizeof(tail_block)') == 86

    with pytest.raises(IndexError):
        dummy = records[2]

    with pytest.raises(ValueError):
        dtree.eval_expr('tail_block.records[2]')


def test_leveldb_log_multiblock(spec_log, data_log_multiblock):
    dtree = model.DataTree(data_log_multiblock, spec_log)
    assert len(dtree.head_blocks) == 1
    assert len(dtree.head_blocks[0].records) == 762
    assert len(dtree.tail_block.records) == 3
    assert dtree.eval_expr('sizeof(head_blocks)') == 32768
    assert dtree.eval_expr('sizeof(head_blocks[0])') == 32768
    assert dtree.eval_expr('sizeof(head_blocks[0].records)') == 32766
    assert dtree.eval_expr('sizeof(head_blocks[0].trailer)') == 2
    assert model.make_python_object(
        dtree.eval_expr('head_blocks[0].trailer')) == '\x00\x00'
    assert dtree.eval_expr('sizeof(tail_block.records)') == 43 * 3

    records = dtree.head_blocks[0].records
    dummy = records[761]
    with pytest.raises(IndexError):
        dummy = records[762]



#
# LDB test (SST structures)
#

@pytest.fixture
def spec_ldb():
    return """
    let FixInt   = integer { signed: false; endian: 'little'; };
    let FixInt8  = byte <> FixInt;
    let FixInt32 = [4] byte <> FixInt;
    let VarInt   = [] byte <> varint;

    let CompressedDataBlock = [] byte <> snappy <> DataBlock;

    let DataBlock = struct {
        entries:     [] KeyValue;
        restarts:    [nb_restarts] FixInt32;
        nb_restarts: FixInt32;
    };

    let KeyValue = struct {
        key_shared_size:     VarInt;
        key_non_shared_size: VarInt;
        value_size:          VarInt;
        key_non_shared:      [key_non_shared_size] byte;
        value:               [value_size] byte;
    };

    let BlockTrailer = struct {
        blocktype: FixInt8;
        crc:       FixInt32;
    };

    let FileBlock = struct {
        if (trailer.blocktype == 0) { // uncompressed
            DataBlock;
        }
        if (trailer.blocktype == 1) {
            CompressedDataBlock;
        }
        trailer: BlockTrailer;
    };

    let BlockHandle = struct {
        offset: VarInt;
        size:   VarInt;

        let ?stored_block =
            file.payload[offset .. offset + size + sizeof(BlockTrailer)]
                 <> FileBlock;
    };

    let Footer = struct {
        metaindex_handle: BlockHandle;
        index_handle:     BlockHandle;
                          [] byte;
        magic:            [8] byte;

        @span: 48;
    };

    file {
        payload: [] byte;
        footer:  Footer;

        let ?index =     footer.index_handle;
        let ?metaindex = footer.metaindex_handle;
    }
    """

@pytest.fixture
def data_ldb():
    return {
        'data': conftest.load_test_dat(__file__, 'test1.ldb'),
        'nb_entries': 237
    }


def test_ldb(spec_ldb, data_ldb):
    data, nb_entries = (data_ldb['data'],
                        data_ldb['nb_entries'])
    dtree = model.DataTree(data, spec_ldb)
    index = dtree.eval_expr('?index')
    # assert index.offset == 265031
    # assert index.size == 5676
    index_block = index['?stored_block']
    # # 5 more bytes than the stored size because block size includes
    # # the trailer
    # assert index_block.get_location() == (265031, 5681)
    assert len(index_block.entries) == nb_entries
    last_index = None
    for i, entry in enumerate(index_block.entries):
        last_index = i
    assert last_index == nb_entries - 1
    assert len(index_block.restarts) == index_block.nb_restarts
    assert index_block.restarts.get_size() == index_block.nb_restarts * 4

    # get a heading child block
    child_handle = index_block.eval_expr('entries[1].value <> BlockHandle')
    assert child_handle.offset == 959
    assert child_handle.size == 1423
    child_block = child_handle['?stored_block']
    assert child_block.get_location() == (959, 1428)
    assert child_block.trailer.blocktype == 1 # compressed
    assert len(child_block.entries) == 5
    assert len(child_block.entries[2].value) == 1022

    # get an intermediate child block
    child_handle = index_block.eval_expr('entries[42].value <> BlockHandle')
    assert child_handle.offset == 33953
    assert child_handle.size == 821
    child_block = child_handle['?stored_block']
    assert child_block.get_location() == (33953, 826)
    assert child_block.trailer.blocktype == 1 # compressed
    assert len(child_block.entries) == 9
    assert len(child_block.entries[4].value) == 479
    # location is relative to the uncompressed block
    assert child_block.entries[4].value.get_location() == (1990, 479)
