
			var container;
			var camera, scene, renderer;
            var modelObj;

			function main(){
				init();
				load3DModel();
				render();
			}
			main();

			/**
			 * 初始化相机、场景、渲染器和光线，和其它例子是一样的，无需特别设置。
			 */
			function init() {
				container = document.body;
				camera = new THREE.PerspectiveCamera( 25, window.innerWidth / window.innerHeight, 1, 10000 );
				camera.position.set( 0, 4, 30 );
				camera.up.set( 0, 1, 0 );
				scene = new THREE.Scene();
				var light = new THREE.DirectionalLight( 0xffffff, 1.5 );
				light.position.set( 0, 0, 20 ).normalize();
				scene.add( light );
				renderer = new THREE.WebGLRenderer( { antialias: true } );
				renderer.setClearColor( 0xfff4e5 );
				renderer.setSize( window.innerWidth, window.innerHeight );
				container.appendChild( renderer.domElement );
			}

			/**
			 * 加载3D模型，所有加载的代码在这里
			 */
			function load3DModel(){
				var loader = new THREE.OBJLoader();
				loader.load( "./threejs/Home02.obj", function ( geometry ) {
					var material = new THREE.MeshLambertMaterial({
        				color:0x5C3A21
    				});

    				geometry.children.forEach(function(child){

        				if(child.children.length==1){
            				if(child.children[0] instanceof THREE.Mesh){
                				child.children[0].material = material;
            				}
        				}
        				modelObj = child;
    				});

    				geometry.scale.set(0.05,0.05,0.05);
    				scene.add(geometry);
				} );
			}

			/**
			 * 渲染
			 */
			function render() {
				/*******************************************
				 *由于加载3D模型是异步的，所以要把render函数放在requestAnimationFrame循环里，
				 * 这样，当模型加载完成后，才能渲染出来
				 ******************************************/
				requestAnimationFrame( render, renderer.domElement );
				if(modelObj){
					modelObj.rotation.y += 0.01;
				}
				renderer.render( scene, camera );
			}

