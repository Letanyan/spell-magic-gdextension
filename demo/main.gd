extends Node2D


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	#var ext = GDExpr.new()
	var res = GDExpr.bake("E + 3 * sin((S + 2) * pi) * Q", {"pi": "3.14", "S": "3+5*cos(pi)", "E": "S+S*pi", "Q": "sin(1+1)"})
	print(res)
	#var temp := GDExpr.bake("10 + sin(2 + 5 * A + 10) + cos(2 * 3)", {})
	#var temp := GDExpr.bake("5 * 2 + 3", {})
	#print(temp)
