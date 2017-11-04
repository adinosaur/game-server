# -*- coding:UTF-8 -*-

import PyGameServer

def call_per_second(param):
	print("call_per_second")

def init():
	PyGameServer.reg_per_second(call_per_second, None)