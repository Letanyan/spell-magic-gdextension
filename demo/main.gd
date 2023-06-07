extends Node2D


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var ext = GDExpr.new()
	var x = 3 + 5 * -5 - -sin(5 + min(1.0, 1))
	ext.build("3 + 5 * -a - -sin(a + min(1.0, alpha))")
	print(ext.compute({"a": 5.0, "alpha": 1.0}))
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	
	
	pass
