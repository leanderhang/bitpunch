#!/usr/bin/env python

from __future__ import print_function
import pytest
import conftest
import json
import sys

import bitpunch.model as model

@pytest.fixture
def spec():
    return """
let u32 = [4] byte <> integer { @signed: false; @endian: 'big'; };

let TinyDBValue = struct {
    flags:      byte <> integer { @signed: false; };
    key_size:   u32;
    key_value:  [key_size] byte <> string;
    value_size: u32;
    value:      [value_size] byte;
};

let Schema = struct {
    values: [] TinyDBValue;
};
"""

@pytest.fixture
def data_ok1():
    tinydb_ok1 = """
00  00 00 00 05 "color"         00 00 00 03 "red"
00  00 00 00 04 "size"          00 00 00 08 "two feet"
00  00 00 00 0B "description"   00 00 00 1C "A nice dwarf with a long axe"
"""

    return conftest.to_bytes(tinydb_ok1)


def test_tinydb(spec, data_ok1):

    board = model.Board()
    board.add_spec('Spec', spec)
    board.add_data_source('data', data_ok1)
    dom = board.eval_expr('data <> Spec.Schema')

    assert len(dom['values']) == 3
    values = dom['values']
    assert str(values[1].key_value) == 'size'
    with pytest.raises(IndexError):
        a = values[3].key_value
        with pytest.raises(TypeError):
            b = values[-1].key_value

    expected_keys = ['color', 'size', 'description']
    niter = 0
    for item, expected_key in zip(values, expected_keys):
        assert str(item.key_value) == expected_key
        niter += 1
    assert niter == 3

    expected_attr_list = ['flags', 'key_size', 'key_value', 'value_size', 'value']
    expected_first_item_items = [
        ('flags', 0),
        ('key_size', 5),
        ('key_value', 'color'),
        ('value_size', 3),
        ('value', 'red') ]
    first_item = True
    for item in values:
        # item is a mapping because TinyDBValue is a 'struct'
        assert list(item.iter_keys()) == expected_attr_list
        if first_item:
            #FIXME
            #assert model.make_python_object(
            #    item.iter_items())) == expected_first_item_items
            first_item = False

    description = [item for item in values
                   if str(item.key_value) == 'description'][0]

    item = values[1]
    assert 'key_value' in item
    assert str(item.key_value) == 'size'
    assert str(item['key_value']) == 'size'
    assert model.make_python_object(item.value) == 'two feet'
    assert model.make_python_object(item['value']) == 'two feet'

    assert 'foo' not in item
    with pytest.raises(AttributeError):
        print(item.foo)
        with pytest.raises(KeyError):
            print(item['foo'])
            with pytest.raises(IndexError):
                print(item[42])

    #FIXME
    #assert values.getAbsDpath() == 'values'
    #assert values[1].getAbsDpath() == 'values[1]'
